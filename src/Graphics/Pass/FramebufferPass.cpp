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

        /*auto&& pFrameBuffer = GetFramebuffer();

        if (!pFrameBuffer) {
            m_isFrameBufferRendered = false;
            return false;
        }

        if (!pFrameBuffer->Bind()) {
            m_isFrameBufferRendered = false;
            return false;
        }

        if (!pFrameBuffer->BeginCmdBuffer(GetClearColors(), GetClearDepth())) {
            m_isFrameBufferRendered = false;
            return false;
        }

        pFrameBuffer->SetViewportScissor();

        if (pFrameBuffer->BeginRender()) {
            GroupPass::Render();
            pFrameBuffer->EndRender();
            pFrameBuffer->EndCmdBuffer();
        }

        GetPipeline()->SetCurrentFrameBuffer(nullptr);

        m_isFrameBufferRendered = true;*/

        /// Независимо от того, отрисовали мы что-то в кадровый буффер или нет,
        /// все равно возвращаем false (hasDrawData), так как технически, кадровый буффер
        /// не несет данных для рендера на экран.
        //return false;
    }

    void FrameBufferPass::Update() {
        m_data.UpdateFrameBuffer([this]() {
            GroupPass::Update();
        });

        /*auto&& pFrameBuffer = GetFramebuffer();
        if (!pFrameBuffer || pFrameBuffer->IsDirty()) {
            return;
        }

        if (!m_isFrameBufferRendered) {
            return;
        }

        GetPassPipeline()->SetCurrentFrameBuffer(pFrameBuffer.Get());

        GroupPass::Update();

        GetPassPipeline()->SetCurrentFrameBuffer(nullptr);*/
    }

    void FrameBufferPass::GetFrameBuffers(FrameBuffers& frameBuffers) const {
        if (auto&& pFrameBuffer = m_data.GetFramebuffer()) {
            frameBuffers.emplace_back(pFrameBuffer);
        }
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

        GetPipeline()->EndRender();

        if (m_clearColor) {
            GetPipeline()->ClearColorBuffer(pFrameBufferPass->GetClearColors());
        }

        if (m_clearDepth) {
            GetPipeline()->ClearDepthBuffer(1.f);
        }

        GetPipeline()->BeginRender();

        return Super::Render();
    }
}