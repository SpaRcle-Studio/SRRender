//
// Created by Monika on 06.10.2026.
//

#include <Graphics/Clouds/VolumetricClouds.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Types/Shader.h>

#include <Utils/World/Scene.h>

#include <Codegen/VolumetricClouds.generated.hpp>

namespace SR_GRAPH_NS {
    void VolumetricClouds::OnEnable() {
        RegisterClouds();
        Super::OnEnable();
    }

    void VolumetricClouds::OnDisable() {
        UnregisterClouds();
        Super::OnDisable();
    }

    void VolumetricClouds::OnDestroy() {
        UnregisterClouds();
        Super::OnDestroy();
    }

    void VolumetricClouds::RegisterClouds() {
        if (m_isRegistered) {
            return;
        }

        auto&& pRenderScene = TryGetRenderScene();
        if (!pRenderScene) {
            return;
        }

        if (pRenderScene->GetVolumetricClouds() && pRenderScene->GetVolumetricClouds() != this) {
            SR_WARN("VolumetricClouds::RegisterClouds() : scene already has active volumetric clouds, replacing.");
        }

        pRenderScene->SetVolumetricClouds(this);
        pRenderScene->SetDirty();
        m_isRegistered = true;
    }

    void VolumetricClouds::UnregisterClouds() {
        if (!m_isRegistered) {
            return;
        }

        m_isRegistered = false;

        if (auto&& pRenderScene = TryGetRenderScene()) {
            if (pRenderScene->GetVolumetricClouds() == this) {
                pRenderScene->SetVolumetricClouds(nullptr);
            }
            pRenderScene->SetDirty();
        }
    }

    VolumetricClouds::RenderScenePtr VolumetricClouds::TryGetRenderScene() const {
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

    void VolumetricClouds::UseUniforms(SR_GTYPES_NS::Shader& shader) const {
        static const SR_UTILS_NS::StringAtom shapeParams = "cloudsShapeParams";
        static const SR_UTILS_NS::StringAtom layerParams = "cloudsLayerParams";
        static const SR_UTILS_NS::StringAtom windParams = "cloudsWindParams";
        static const SR_UTILS_NS::StringAtom lightParams = "cloudsLightParams";
        static const SR_UTILS_NS::StringAtom detailParams = "cloudsDetailParams";

        const float_t windLength = m_windDirection.Length();
        const SR_MATH_NS::FVector2 wind = windLength > 0.0001f ? m_windDirection / windLength : SR_MATH_NS::FVector2(1.f, 0.f);

        shader.SetVec4(shapeParams, SR_MATH_NS::FVector4(m_coverage, m_density, m_cloudType, m_shapeScale));
        shader.SetVec4(layerParams, SR_MATH_NS::FVector4(m_bottomHeight, SR_MATH_NS::Max(m_thickness, 1.f), m_maxDistance, 0.f));
        shader.SetVec4(windParams, SR_MATH_NS::FVector4(wind.x, wind.y, m_windSpeed, 0.f));
        shader.SetVec4(lightParams, SR_MATH_NS::FVector4(m_lightAbsorption, m_ambientStrength, m_silverLining, m_powderStrength));
        shader.SetVec4(detailParams, SR_MATH_NS::FVector4(
            m_detailScale,
            m_detailStrength,
            static_cast<float_t>(SR_MATH_NS::Clamp<uint32_t>(m_primarySteps, 8, 128)),
            static_cast<float_t>(SR_MATH_NS::Clamp<uint32_t>(m_lightSteps, 1, 10))
        ));
    }
}
