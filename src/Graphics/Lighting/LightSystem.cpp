//
// Created by Monika on 22.05.2023.
//

#include <Graphics/Render/RenderScene.h>
#include <Graphics/Lighting/LightSystem.h>
#include <Graphics/Lighting/ILightComponent.h>
#include <Graphics/Lighting/DirectionalLight.h>
#include <Graphics/Types/Mesh.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Pipeline/ShaderUtils.h>

#include <Utils/Math/Curve.h>

namespace SR_GRAPH_NS {
    LightSystem::LightSystem(RenderScenePtr pRenderScene)
        : Super()
        , m_renderScene(pRenderScene)
    {
        m_directionalLightParams.direction = SR_MATH_NS::FVector3(20, 60, 5).Normalize();
        m_directionalLightParams.lightColor = SR_MATH_NS::FColor(1.0f, 0.95f, 0.85f);

        m_secondaryDirectionalLightParams.direction = -m_directionalLightParams.direction;
        m_secondaryDirectionalLightParams.intensity = 0.f;
        m_secondaryDirectionalLightParams.diskIntensity = 0.f;
    }

    LightSystem::~LightSystem() {
        for (auto& lightSet : m_lights) {
            SRAssert(lightSet.empty());
        }
    }

    void LightSystem::Register(ILightComponent* pLightComponent) {
        if (!SRVerify(pLightComponent)) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        const uint32_t index = SR_UTILS_NS::EnumReflector::AsInt(pLightComponent->GetLightType());

        if (m_lights[index].find(pLightComponent) != m_lights[index].end()) {
            SRHalt("LightSystem::Register() : light component is already registered!");
            return;
        }

        m_lights[index].insert(pLightComponent);
        m_renderScene->SetDirty();

        pLightComponent->UpdateLightParams();
        OnLightChanged(pLightComponent);
    }

    void LightSystem::Remove(ILightComponent* pLightComponent) {
        if (!SRVerify(pLightComponent)) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        const uint32_t index = SR_UTILS_NS::EnumReflector::AsInt(pLightComponent->GetLightType());

        auto&& pIt = m_lights[index].find(pLightComponent);
        if (pIt == m_lights[index].end()) {
            SRHalt("LightSystem::Remove() : light component is not registered!");
            return;
        }

        m_lights[index].erase(pIt);

        if (pLightComponent->GetLightType() == LightType::Directional) {
            UpdateDirectionalLights();
        }

        m_renderScene->SetDirty();
    }

    void LightSystem::OnLightChanged(ILightComponent* pLightComponent) {
        SR_TRACY_ZONE;

        if (!SRVerify(pLightComponent)) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        const auto type = pLightComponent->GetLightType();
        const uint32_t index = SR_UTILS_NS::EnumReflector::AsInt(type);
        auto&& pIt = m_lights[index].find(pLightComponent);
        if (pIt == m_lights[index].end()) {
            SRHalt("LightSystem::OnLightChanged() : light component is not registered!");
            return;
        }

        if (type == LightType::Directional) {
            UpdateDirectionalLights();
        }

        m_renderScene->SetDirty();
    }

    const DirectionalLightParams& LightSystem::GetDirectionalLightParams() const noexcept {
        return m_directionalLightParams;
    }

    const DirectionalLightParams& LightSystem::GetSecondaryDirectionalLightParams() const noexcept {
        return m_secondaryDirectionalLightParams;
    }

    void LightSystem::UpdateDirectionalLights() {
        /// Основной свет (тени, ambient) - самый яркий в данный момент, как солнце днем и луна ночью.
        /// Второй по яркости рисуется на небе и учитывается облаками.
        auto&& lights = m_lights[SR_UTILS_NS::EnumReflector::AsInt(LightType::Directional)];

        const DirectionalLight* pPrimary = nullptr;
        const DirectionalLight* pSecondary = nullptr;

        for (ILightComponent* pLight : lights) {
            auto&& pDirectional = static_cast<const DirectionalLight*>(pLight);
            const float_t intensity = pDirectional->GetParams().intensity + pDirectional->GetParams().environmentWeight * 0.01f;

            auto&& weightOf = [](const DirectionalLight* pL) {
                return pL->GetParams().intensity + pL->GetParams().environmentWeight * 0.01f;
            };

            if (!pPrimary || intensity > weightOf(pPrimary)) {
                pSecondary = pPrimary;
                pPrimary = pDirectional;
            }
            else if (!pSecondary || intensity > weightOf(pSecondary)) {
                pSecondary = pDirectional;
            }
        }

        if (pPrimary) {
            m_directionalLightParams = pPrimary->GetParams();
        }

        m_hasSecondaryDirectionalLight = pSecondary != nullptr;
        m_secondaryDirectionalLightParams = pSecondary ? pSecondary->GetParams() : DirectionalLightParams();
        if (!pSecondary) {
            /// нулевое направление дает NaN в шейдерах неба
            m_secondaryDirectionalLightParams.intensity = 0.f;
            m_secondaryDirectionalLightParams.diskIntensity = 0.f;
            m_secondaryDirectionalLightParams.direction = -m_directionalLightParams.direction;
        }

        /// Плавный переход день/ночь: окружение (небо, земля, ambient) смешивается по вкладу обоих светил,
        /// а не переключается скачком, когда меняется самый яркий свет.
        if (pPrimary && pSecondary) {
            auto&& a = pPrimary->GetParams();
            auto&& b = pSecondary->GetParams();
            const float_t sum = a.environmentWeight + b.environmentWeight;
            const float_t w = sum > 1e-5f ? a.environmentWeight / sum : 1.f;

            m_directionalLightParams.skyColor = SR_MATH_NS::Mix(b.skyColor, a.skyColor, w);
            m_directionalLightParams.groundColor = SR_MATH_NS::Mix(b.groundColor, a.groundColor, w);
            m_directionalLightParams.ambientIntensity = SR_MATH_NS::Mix(b.ambientIntensity, a.ambientIntensity, w);
        }

        /// тень слабого света (луна) не должна быть такой же контрастной, как от солнца
        if (pPrimary) {
            const float_t shadowVisibility = SR_MATH_NS::Curve::SmoothStep(0.f, 0.6f, m_directionalLightParams.intensity);
            m_directionalLightParams.shadowStrength *= SR_MATH_NS::Mix(0.25f, 1.f, shadowVisibility);
        }

        /// звезды видны, когда все излучающие светила (солнце) под горизонтом
        float_t emitterHeight = -1.f;
        for (ILightComponent* pLight : lights) {
            auto&& params = static_cast<const DirectionalLight*>(pLight)->GetParams();
            if (params.shading == CelestialBodyShading::Emission) {
                emitterHeight = SR_MATH_NS::Max(emitterHeight, -params.direction.y);
            }
        }
        m_directionalLightParams.starsIntensity = 1.f - SR_MATH_NS::Curve::SmoothStep(-0.12f, 0.05f, emitterHeight);
    }

    void LightSystem::UseSkyUniforms(SR_GTYPES_NS::Shader& shader) const {
        auto&& makeDiskParams = [](const DirectionalLightParams& params, float_t stars) {
            const float_t radius = SR_MATH_NS::Max(params.angularDiameter, 0.01f) * 0.5f * static_cast<float_t>(SR_PI) / 180.f;
            return SR_MATH_NS::FVector4(
                std::cos(radius),
                params.diskIntensity,
                static_cast<float_t>(SR_UTILS_NS::EnumReflector::AsInt(params.shading)),
                stars
            );
        };

        auto&& primary = m_directionalLightParams;
        auto&& secondary = m_secondaryDirectionalLightParams;

        shader.SetVec3(SHADER_SECONDARY_LIGHT_DIRECTION, secondary.direction);
        shader.SetVec3(SHADER_PRIMARY_SKY_ILLUMINANCE, primary.skyIlluminance.RGB());
        shader.SetVec3(SHADER_SECONDARY_SKY_ILLUMINANCE, secondary.skyIlluminance.RGB());
        shader.SetVec3(SHADER_PRIMARY_DISK_COLOR, primary.diskColor.RGB());
        shader.SetVec3(SHADER_SECONDARY_DISK_COLOR, secondary.diskColor.RGB());
        shader.SetVec4(SHADER_PRIMARY_DISK_PARAMS, makeDiskParams(primary, primary.starsIntensity));
        shader.SetVec4(SHADER_SECONDARY_DISK_PARAMS, makeDiskParams(secondary, primary.starsIntensity));
    }
}
