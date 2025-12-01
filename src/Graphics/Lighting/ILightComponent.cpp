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
        if (auto&& pRenderScene = GetRenderScene()) {
            pRenderScene->GetLightSystem()->Register(this);
            m_isLightRegistered = true;
        }
        Super::OnAttached();
    }

    void ILightComponent::OnDestroy() {
        if (auto&& pRenderScene = TryGetRenderScene()) {
            pRenderScene->GetLightSystem()->Remove(this);
            m_isLightRegistered = false;
        }

        Super::OnDestroy();
    }

    void ILightComponent::OnMatrixDirty() {
        if (!m_isLightRegistered) {
            return;
        }

        if (auto&& pRenderScene = TryGetRenderScene()) {
            pRenderScene->GetLightSystem()->OnLightTransformChanged(this);
        }
    }
}