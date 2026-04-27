//
// Created by Monika on 22.07.2022.
//

#include <Graphics/Pass/FrameBufferPass.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Codegen/FrameBufferPass.generated.hpp>

namespace SR_GRAPH_NS {
    bool FrameBufferPass::Render() {
        SR_MAYBE_UNUSED auto frameBufferDebugName = m_data.GetFrameBufferName().c_str();

        if (m_data.RenderFrameBuffer([this]() { return GroupPass::Render(); })) {
            return true;
        }
        return false;
    }

    void FrameBufferPass::Update() {
        SR_TRACY_ZONE;

        m_data.UpdateFrameBuffer([this]() {
            GroupPass::Update();
        });
    }

    const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>& FrameBufferPass::GetFrameBuffer() const noexcept {
        return m_data.GetFramebuffer();
    }

    const FrameBufferPassData::ClearColors& FrameBufferPass::GetClearColors() const noexcept {
        return m_data.GetClearColors();
    }

    void FrameBufferPass::SetRenderTechnique(IRenderTechnique* pRenderTechnique) {
        m_data.SetRenderTechnique(pRenderTechnique);
        Super::SetRenderTechnique(pRenderTechnique);
    }

    /// ----------------------------------------------------------------------------------------------------------------

    bool ClearBuffersPass::Render() {
        SR_ERROR("Deprecated! Use ClearDepthAttachmentPass instead.");

        auto&& pFrameBufferPass = GetFrameBufferPass();
        if (!pFrameBufferPass) {
            SR_WARN("ClearBuffersPass::Render() : parent is not FrameBufferPass!");
            return false;
        }

        auto&& pFBO = pFrameBufferPass->GetFrameBuffer();
        if (!pFBO) {
            return false;
        }

        // Use vkCmdClearAttachments for depth clearing inside active RenderPass
        // This is the correct Vulkan way and avoids READ_AFTER_WRITE hazards
        if (m_clearDepth) {
            GetPipeline()->ClearDepthAttachment(1.f);
        }

        // Color clearing still uses transfer operations if needed
        // (This is less common, but kept for compatibility)
        if (m_clearColor && pFBO->GetFeatures().colorTransferDst) {
            GetPipeline()->EndRender();
            GetPipeline()->ClearColorBuffer(pFrameBufferPass->GetClearColors());
            GetPipeline()->BeginRender();
        }

        return Super::Render();
    }

    /// ----------------------------------------------------------------------------------------------------------------

    bool ClearDepthAttachmentPass::Render() {
        //auto&& pFrameBufferPass = GetFrameBufferPass();
        //if (!pFrameBufferPass) {
        //    SRHalt("ClearDepthAttachmentPass::Render() : parent is not FrameBufferPass!");
        //    return false;
        //}

        //auto&& pFBO = pFrameBufferPass->GetFrameBuffer();
        //if (!pFBO) {
        //    return false;
        //}

        GetPipeline()->ClearDepthAttachment(m_depth);

        return Super::Render();
    }
}