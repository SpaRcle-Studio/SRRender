//
// Created by Nikita on 13.12.2020.
//

#include <Graphics/Lighting/LightSystem.h>
#include <Graphics/Lighting/DirectionalLight.h>
#include <Graphics/Render/RenderScene.h>

namespace SR_GRAPH_NS {
    void DirectionalLight::OnAttached() {
        if (auto&& pRenderScene = GetRenderScene()) {
            pRenderScene->GetLightSystem()->Register(this);
        }
        Component::OnAttached();
    }
}
