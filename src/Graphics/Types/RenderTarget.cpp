//
// Created by Monika on 17.04.2026.
//

#include <Graphics/Types/RenderTarget.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Pass/FrameBufferPass.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/IRenderTechnique.h>

#include <Utils/FileSystem/PathDataAccessor.h>
#include <Utils/World/Scene.h>
#include <Utils/Types/DataStorage.h>

#include <Codegen/RenderTarget.generated.hpp>

namespace SR_GTYPES_NS {
    void RenderTarget::Update(float dt) {
        Activate();
        Super::OnEnable();
    }

    void RenderTarget::OnDisable() {
        Deactivate();
        Super::OnDisable();
    }

    void RenderTarget::OnDetached() {
        Deactivate();
        Super::OnDetached();
    }

    RenderScene* RenderTarget::GetRenderScene() const {
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

    void RenderTarget::Activate() {
        SR_TRACY_ZONE;

        if (m_usedRenderTechnique || m_name.empty()) {
            return;
        }

        auto&& pRenderTechnique = GetRenderTechnique();
        if (!pRenderTechnique) {
            return;
        }

        if (m_resolutionScale.HasZero() || m_resolutionScale.HasNegative()) {
            SR_ERROR("RenderTarget::Activate() : invalid resolution scale: {}x{}", m_resolutionScale.x, m_resolutionScale.y);
            return;
        }

        if (!m_dynamicResolution && m_resolution.HasZero()) {
            SR_ERROR("RenderTarget::Activate() : invalid resolution: {}x{}", m_resolution.x, m_resolution.y);
            return;
        }

        if (pRenderTechnique->GetFrameBufferController(m_name)) {
            SR_ERROR("RenderTarget::Activate() : render technique already has framebuffer controller with name: {}", m_name);
            return;
        }

        auto&& data = pRenderTechnique->GetRenderTechniqueData();

        FrameBufferController::Ptr pFrameBufferController = new FrameBufferController();
        {
            pFrameBufferController->SetName(m_name);
            pFrameBufferController->SetPreScale(m_resolutionScale);
            if (m_dynamicResolution) {
                pFrameBufferController->SetSize(m_resolution.CastToInt());
            }
            pFrameBufferController->SetSamples(m_sampleCount);
            pFrameBufferController->SetDynamicResizing(m_dynamicResolution);

            for (auto&& layer : m_layers) {
                m_cachedFormats.emplace_back(layer.format);
            }
            pFrameBufferController->SetColorFormats(m_cachedFormats);
            m_cachedFormats.clear();

            data.frameBuffers.emplace_back(pFrameBufferController);
        }

        FrameBufferPass::Ptr pFrameBufferPass = new FrameBufferPass();
        {
            pFrameBufferPass->SetFrameBufferName(m_name);
            pFrameBufferPass->SetCustomName(m_name);
            for (auto&& pBasePass : m_passes) {
                if (!pBasePass) {
                    continue;
                }
                auto&& pClone = m_cachedPasses.emplace_back();
                pClone = SR_UTILS_NS::Factory::Instance().Create<BasePass>(pBasePass->GetMeta()->GetFactoryName());
                pBasePass->CloneTo(*pClone);
            }
            pFrameBufferPass->SetPasses(m_cachedPasses);
            pFrameBufferPass->GetFrameBufferPassData().GetClearColors().clear();
            for (auto&& layer : m_layers) {
                pFrameBufferPass->GetFrameBufferPassData().GetClearColors().emplace_back(layer.clearColor);
            }
            pFrameBufferPass->GetFrameBufferPassData().SetClearDepth(m_clearDepth);
            m_cachedPasses.clear();
            data.AddPass(pFrameBufferPass.StaticCast<BasePass>());
        }

        pRenderTechnique->AttachRenderTarget(this);
        m_usedRenderTechnique = pRenderTechnique;
    }

    void RenderTarget::Deactivate() {
        SR_TRACY_ZONE;

        if (!m_usedRenderTechnique) {
            return;
        }

        auto&& data = m_usedRenderTechnique->GetRenderTechniqueData();
        auto&& frameBuffers = data.frameBuffers;
        auto it = std::find_if(frameBuffers.begin(), frameBuffers.end(), [this](const FrameBufferController::Ptr& pController) {
            return pController->GetName() == m_name;
        });
        if (it == frameBuffers.end()) {
            SR_ERROR("RenderTarget::Deactivate() : framebuffer controller with name {} not found in render technique!", m_name);
        }
        else {
            (*it).AutoFree();
            frameBuffers.erase(it);
        }

        if (auto&& pGroupPass = data.pass.DynamicCast<GroupPass>()) {
            pGroupPass->RemovePass(m_name);
        }

        m_usedRenderTechnique->DetachRenderTarget(this);
        m_usedRenderTechnique = nullptr;
    }

    IRenderTechnique* RenderTarget::GetRenderTechnique() const {
        SR_TRACY_ZONE;
        Camera::Ptr pCamera = m_camera.Get();
        if (!pCamera) {
            if (auto&& pRenderScene = GetRenderScene()) {
                pCamera = pRenderScene->GetMainCamera();
            }
            if (!pCamera) {
                return nullptr;
            }
        }
        return pCamera->GetRenderTechnique();
    }
}