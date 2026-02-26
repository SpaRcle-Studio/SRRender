//
// Created by Monika on 14.09.2025.
//

#include <Graphics/Loaders/RenderTechniquePostProcess.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/IRenderTechnique.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Pass/SkyboxPass.h>
#include <Graphics/Pass/FrameBufferPass.h>
#include <Graphics/Pass/SwapchainPass.h>

namespace SR_GRAPH_NS::Details {
    bool ReplaceSwapchainPass(BasePass::Ptr& pPass, SR_UTILS_NS::StringAtom name) {
        SR_TRACY_ZONE;

        if (!pPass) {
            return false;
        }

        if (auto pSwapchain = pPass.DynamicCast<SwapchainPass>()) {
            pPass = SRNew<FrameBufferPass>();
            auto&& pFrameBufferPass = pPass.StaticCast<FrameBufferPass>();

            pFrameBufferPass->SetPasses(pSwapchain->GetPasses());
            pFrameBufferPass->SetCustomName(name);
            pFrameBufferPass->GetFrameBufferPassData().SetFrameBufferName(name);
            pFrameBufferPass->GetFrameBufferPassData().SetClearColors({ pSwapchain->GetClearColor() });
            pFrameBufferPass->GetFrameBufferPassData().SetClearDepth(pSwapchain->GetClearDepth());

            pSwapchain->GetPasses().clear();
            return true;
        }

        if (auto&& pGroupPass = pPass.DynamicCast<GroupPass>()) {
            for (auto&& pChild : pGroupPass->GetPasses()) {
                if (ReplaceSwapchainPass(pChild, name)) {
                    return true;
                }
            }
        }

        return false;
    }

    void PostProcessRenderTechnique(IRenderTechnique* pTechnique, RenderContext* pContext, SR_GTYPES_NS::CameraType cameraType) {
        SR_TRACY_ZONE;

        if (!pTechnique || !pContext) {
            return;
        }

        const bool isEditor = cameraType == SR_GTYPES_NS::CameraType::Editor || cameraType == SR_GTYPES_NS::CameraType::EditorPrefab;

        if (isEditor && !pTechnique->FindPass(pContext->GetSettings().editorSceneImageName)) {
            auto&& data = pTechnique->GetRenderTechniqueData();
            if (ReplaceSwapchainPass(data.pass, pContext->GetSettings().editorSceneImageName)) {
                auto&& pFBO = data.frameBuffers.emplace_back(SRNew<FrameBufferController>());
                pFBO->SetName(pContext->GetSettings().editorSceneImageName);

                pTechnique->OnHierarchyChanged();
            }
        }
    }
}