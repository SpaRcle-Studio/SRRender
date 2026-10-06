//
// Created by Monika on 06.10.2026.
//

#include <Graphics/Fog/VolumetricFog.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Types/Shader.h>

#include <Utils/World/Scene.h>

#include <Codegen/VolumetricFog.generated.hpp>

namespace SR_GRAPH_NS {
    void VolumetricFog::OnEnable() {
        RegisterFog();
        Super::OnEnable();
    }

    void VolumetricFog::OnDisable() {
        UnregisterFog();
        Super::OnDisable();
    }

    void VolumetricFog::OnDestroy() {
        UnregisterFog();
        Super::OnDestroy();
    }

    void VolumetricFog::RegisterFog() {
        if (m_isRegistered) {
            return;
        }

        auto&& pRenderScene = TryGetRenderScene();
        if (!pRenderScene) {
            return;
        }

        if (pRenderScene->GetVolumetricFog() && pRenderScene->GetVolumetricFog() != this) {
            SR_WARN("VolumetricFog::RegisterFog() : scene already has active volumetric fog, replacing.");
        }

        pRenderScene->SetVolumetricFog(this);
        pRenderScene->SetDirty();
        m_isRegistered = true;
    }

    void VolumetricFog::UnregisterFog() {
        if (!m_isRegistered) {
            return;
        }

        m_isRegistered = false;

        if (auto&& pRenderScene = TryGetRenderScene()) {
            if (pRenderScene->GetVolumetricFog() == this) {
                pRenderScene->SetVolumetricFog(nullptr);
            }
            pRenderScene->SetDirty();
        }
    }

    VolumetricFog::RenderScenePtr VolumetricFog::TryGetRenderScene() const {
        if (m_renderScene) {
            return m_renderScene;
        }
        auto&& pScene = TryGetScene();
        if (!pScene) {
            return m_renderScene;
        }
        m_renderScene = dynamic_cast<RenderScene*>(pScene->GetModule("Render"));
        return m_renderScene;
    }

    void VolumetricFog::UseUniforms(SR_GTYPES_NS::Shader& shader) const {
        static const SR_UTILS_NS::StringAtom densityParams = "fogDensityParams";
        static const SR_UTILS_NS::StringAtom colorParams = "fogColorParams";
        static const SR_UTILS_NS::StringAtom lightParams = "fogLightParams";
        static const SR_UTILS_NS::StringAtom noiseParams = "fogNoiseParams";

        const float_t windLength = m_windDirection.Length();
        const SR_MATH_NS::FVector2 wind = windLength > 0.0001f ? m_windDirection / windLength : SR_MATH_NS::FVector2(1.f, 0.f);

        shader.SetVec4(densityParams, SR_MATH_NS::FVector4(
            SR_MATH_NS::Max(m_density, 0.f),
            m_baseHeight,
            SR_MATH_NS::Max(m_heightFalloff, 0.0001f),
            SR_MATH_NS::Max(m_maxDistance, 1.f)
        ));
        shader.SetVec4(colorParams, SR_MATH_NS::FVector4(m_albedo.r, m_albedo.g, m_albedo.b, m_ambientStrength));
        shader.SetVec4(lightParams, SR_MATH_NS::FVector4(
            SR_MATH_NS::Clamp(m_anisotropy, -0.95f, 0.95f),
            m_lightShaftsIntensity,
            m_shadows ? 1.f : 0.f,
            m_skyColorInfluence
        ));
        shader.SetVec4(noiseParams, SR_MATH_NS::FVector4(m_noiseStrength, m_noiseScale, m_windSpeed, SR_MATH_NS::Max(m_startDistance, 0.f)));

        static const SR_UTILS_NS::StringAtom windParams = "fogWindParams";
        shader.SetVec4(windParams, SR_MATH_NS::FVector4(
            wind.x,
            wind.y,
            static_cast<float_t>(SR_MATH_NS::Clamp<uint32_t>(m_steps, 8, 64)),
            0.f
        ));
    }
}
