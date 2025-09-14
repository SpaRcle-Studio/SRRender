//
// Created by Monika on 22.07.2022.
//

#include <Graphics/Pass/FrameBufferPass.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Codegen/FrameBufferPass.generated.hpp>

namespace SR_GRAPH_NS {
    bool FrameBufferPass::Render() {
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
        auto&& pFrameBufferPass = dynamic_cast<FrameBufferPass*>(GetParent());
        if (!pFrameBufferPass) {
            SR_WARN("ClearBuffersPass::Render() : parent is not FrameBufferPass!");
            return false;
        }

        auto&& pFBO = pFrameBufferPass->GetFrameBuffer();
        if (!pFBO) {
            return false;
        }

        GetPipeline()->EndRender();

        if (m_clearColor && pFBO->GetFeatures().colorTransferDst) {
            GetPipeline()->ClearColorBuffer(pFrameBufferPass->GetClearColors());
        }

        if (m_clearDepth && pFBO->GetFeatures().depthTransferDst) {
            GetPipeline()->ClearDepthBuffer(1.f);
        }

        GetPipeline()->BeginRender();

        return Super::Render();
    }
}