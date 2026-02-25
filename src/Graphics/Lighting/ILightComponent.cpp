//
// Created by Nikita on 13.12.2020.
//

#include <Graphics/Lighting/LightSystem.h>
#include <Graphics/Lighting/ILightComponent.h>
#include <Graphics/Render/RenderScene.h>

#include <Enum/LightType.hpp>

#include <Codegen/ILightComponent.generated.hpp>

namespace SR_GRAPH_NS {
    void ILightComponent::OnAttached() {
        Super::OnAttached();
    }

    void ILightComponent::OnDestroy() {
        UnregisterLight();
        Super::OnDestroy();
    }

    void ILightComponent::OnMatrixDirty() {
        if (!m_isLightRegistered) {
            return;
        }
        UpdateLightParams();
    }

    void ILightComponent::OnEnable() {
        RegisterLight();
        Super::OnEnable();
    }

    void ILightComponent::OnDisable() {
        UnregisterLight();
        Super::OnDisable();
    }

    void ILightComponent::UnregisterLight() {
        if (!m_isLightRegistered) {
            return;
        }
        if (auto&& pRenderScene = TryGetRenderScene()) {
            pRenderScene->GetLightSystem()->Remove(this);
            m_isLightRegistered = false;
        }
    }

    void ILightComponent::RegisterLight() {
        if (m_isLightRegistered) {
            return;
        }
        if (auto&& pRenderScene = GetRenderScene()) {
            m_isLightRegistered = true;
            pRenderScene->GetLightSystem()->Register(this);
        }
    }

    void ILightComponent::UpdateLightParams() {
        SR_TRACY_ZONE;

        if (!m_parent || !GetTransform() || !m_isLightRegistered) {
            return;
        }

        UpdateLightParamsImpl();

        if (auto&& pRenderScene = TryGetRenderScene()) {
            pRenderScene->GetLightSystem()->OnLightChanged(this);
        }
    }

    void ILightComponent::OnPostLoad() {
        SR_TRACY_ZONE;
        UpdateLightParams();
        Super::OnPostLoad();
    }
}