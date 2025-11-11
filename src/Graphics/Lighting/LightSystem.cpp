//
// Created by Monika on 22.05.2023.
//

#include <Graphics/Render/RenderScene.h>
#include <Graphics/Lighting/LightSystem.h>
#include <Graphics/Lighting/ILightComponent.h>
#include <Graphics/Types/Mesh.h>

namespace SR_GRAPH_NS {
    LightSystem::LightSystem(RenderScenePtr pRenderScene)
        : Super()
        , m_renderScene(pRenderScene)
    {
        m_directionalLightDir = SR_MATH_NS::FVector3(20, 60, 5).Normalize();
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

        OnLightTransformChanged(pLightComponent);
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
        m_renderScene->SetDirty();
    }

    void LightSystem::OnLightTransformChanged(ILightComponent* pLightComponent) {
        SR_TRACY_ZONE;

        if (!SRVerify(pLightComponent)) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        const auto type = pLightComponent->GetLightType();
        const uint32_t index = SR_UTILS_NS::EnumReflector::AsInt(type);
        auto&& pIt = m_lights[index].find(pLightComponent);
        if (pIt == m_lights[index].end()) {
            SRHalt("LightSystem::OnLightTransformChanged() : light component is not registered!");
            return;
        }

        if (type == LightType::Directional) {
            m_directionalLightDir = -pLightComponent->GetTransform()->Forward().Normalize();
        }

        m_renderScene->SetDirty();
        m_renderScene->GetRenderStrategy()->ForEachMesh([](SR_GTYPES_NS::Mesh* pMesh) {
            pMesh->MarkUniformsDirty();
        });
    }

    SR_MATH_NS::FVector3 LightSystem::GetDirectionalLightDirection() const noexcept {
        return m_directionalLightDir;
    }
}
