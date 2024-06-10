//
// Created by Nikita on 13.12.2020.
//

#include <Graphics/Lighting/LightSystem.h>
#include <Graphics/Lighting/ILightComponent.h>

namespace SR_GRAPH_NS {

    void ILightComponent::OnAttached() {
        if (auto&& pRenderScene = GetRenderScene()) {
            pRenderScene->GetLightSystem()->Register(this);
        }
        Component::OnAttached();
    }

    void ILightComponent::OnDestroy() {
        RenderScene* pRenderScene = TryGetRenderScene();

        Component::OnDestroy();

        if (pRenderScene) {
            pRenderScene->GetLightSystem()->Remove(this);
        }
    }
}