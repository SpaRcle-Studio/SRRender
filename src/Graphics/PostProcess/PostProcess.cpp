//
// Created by Monika on 13.02.2026.
//

#include <Graphics/PostProcess/PostProcess.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/IRenderTechnique.h>
#include <Graphics/Pass/SkyboxPass.h>

#include <Utils/FileSystem/PathDataAccessor.h>
#include <Utils/World/Scene.h>
#include <Utils/Types/DataStorage.h>

#include <Codegen/PostProcess.generated.hpp>

namespace SR_GTYPES_NS {
    /*void PostProcessComponent::OnDisable() {
        Super::OnDisable();
    }

    void PostProcessComponent::OnDetached() {
        Super::OnDetached();
    }

    RenderScene* PostProcessComponent::GetRenderScene() const {
        if (m_renderScene) {
            return m_renderScene.Get();
        }

        if (auto&& pScene = TryGetScene()) {
            if (auto&& pRenderScene = pScene->GetDataStorage().GetValue<RenderScene::Ptr>()) {
                m_renderScene = pRenderScene;
            }
        }
        return m_renderScene.Get();
    }

    void PostProcessComponent::Update(float_t dt) {
        SR_TRACY_ZONE;

        Super::Update(dt);
    }*/
}
