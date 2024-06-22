//
// Created by Monika on 22.05.2023.
//

#include <Utils/World/Scene.h>

#include <Graphics/Types/IRenderComponent.h>
#include <Graphics/Render/RenderScene.h>

namespace SR_GTYPES_NS {
    void IRenderComponent::OnEnable() {
        if (auto&& renderScene = TryGetRenderScene()) {
            renderScene->SetDirty();
        }

        Super::OnEnable();
    }

    void IRenderComponent::OnDisable() {
        if (auto&& renderScene = TryGetRenderScene()) {
            renderScene->SetDirty();
        }

        Super::OnDisable();
    }

    IRenderComponent::CameraPtr IRenderComponent::GetCamera() const {
        if (auto&& pRenderScene = TryGetRenderScene()) {
            return pRenderScene->GetMainCamera();
        }
        return CameraPtr();
    }

    IRenderComponent::RenderScenePtr IRenderComponent::TryGetRenderScene() const  {
        if (m_renderScene) {
            return m_renderScene;
        }

        auto&& pScene = TryGetScene();
        if (!pScene) {
            return m_renderScene;
        }

        m_renderScene = pScene->GetDataStorage().GetPointer<RenderScene>();

        return m_renderScene;
    }

    IRenderComponent::RenderScenePtr IRenderComponent::GetRenderScene() const {
        if (auto&& pRenderScene = TryGetRenderScene()) {
            return pRenderScene;
        }

        SRHalt("Invalid render scene!");

        return RenderScenePtr();
    }
}