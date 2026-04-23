//
// Created by Monika on 21.01.2023.
//

#include <Graphics/Pass/Data/FrameBufferPassData.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/FrameBufferController.h>
#include <Graphics/Render/IRenderTechnique.h>
#include <Graphics/Types/Framebuffer.h>

#include <Codegen/FrameBufferPassData.generated.hpp>

namespace SR_GRAPH_NS {
    FrameBufferPassData::FrameBufferPassData()
        : Super()
    { }

    bool FrameBufferPassData::RenderFrameBuffer(const FBRenderCallback& callback) {
        m_isFrameBufferRendered = false;

        auto&& pFrameBuffer = GetFramebuffer();
        if (!pFrameBuffer) {
            SR_ERROR("FrameBufferPassData::RenderFrameBuffer() : frame buffer is not found!\n\tName: {}", m_frameBufferName);
            return false;
        }

        if (pFrameBuffer->IsDepthEnabled() && !m_depth.has_value()) {
            SR_ERROR("FrameBufferPassData::RenderFrameBuffer() : depth is not set!\n\tName: {}", m_frameBufferName);
            return false;
        }

        if (!pFrameBuffer->Update()) {
            return false;
        }

        if (!pFrameBuffer->IsRenderAsSingleLayer() && GetLayersCount() > 1) {
            return RenderFrameBuffer(callback, GetLayersCount());
        }

        if (pFrameBuffer->Bind()) {
            pFrameBuffer->ClearBuffers(m_clearColors, m_depth);
            pFrameBuffer->BeginRender();
            {
                pFrameBuffer->SetViewportScissor();
                callback();
                m_isFrameBufferRendered = true;
            }
            pFrameBuffer->EndRender();
        }

        //if (pFrameBuffer->GetFeatures().colorShaderRead) {
        //    /// memory barrier чтобы гарантировать видимость данных в шейдерах
        //    GetPipeline()->MemoryBarrier(Pipeline::MemoryBarrierBits::ShaderRead);
        //}

        GetPipeline()->SetCurrentFrameBuffer(nullptr);

        return false;
    }

    bool FrameBufferPassData::RenderFrameBuffer(const FBRenderCallback& callback, uint8_t layers) {
        auto&& pFrameBuffer = GetFramebuffer();
        auto&& pPipeline = GetPipeline();

        /// установим кадровый буфер, чтобы BeginCmdBuffer понимал какие значение для очистки ставить
        pPipeline->SetCurrentFrameBuffer(const_cast<Pipeline::FramebufferPtr>(pFrameBuffer.Get()));

        pFrameBuffer->ClearBuffers(m_clearColors, m_depth);
        pFrameBuffer->SetViewportScissor();

        for (uint32_t i = 0; i < layers; ++i) {
            GetPipeline()->SetCurrentFrameBufferLayer(i);

            if (pFrameBuffer->Bind()) {
                pFrameBuffer->BeginRender();

                callback();
                m_isFrameBufferRendered = true;

                pFrameBuffer->EndRender();
            }
        }

        GetPipeline()->SetCurrentFrameBuffer(nullptr);

        return false;
    }

    void FrameBufferPassData::UpdateFrameBuffer(const FBUpdateCallback& callback) {
        if (!m_isFrameBufferRendered) {
            return;
        }

        auto&& pFrameBuffer = GetFramebuffer();
        if (!pFrameBuffer || pFrameBuffer->IsDirty()) {
            return;
        }

        GetPipeline()->SetCurrentFrameBuffer(const_cast<Pipeline::FramebufferPtr>(pFrameBuffer.Get()));

        const uint32_t layersCount = pFrameBuffer->IsRenderAsSingleLayer() ? 1 : GetLayersCount();
        for (uint32_t i = 0; i < layersCount; ++i) {
            GetPipeline()->SetCurrentFrameBufferLayer(i);
            callback();
        }

        GetPipeline()->SetCurrentFrameBuffer(nullptr);
    }

    const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>& FrameBufferPassData::GetFramebuffer() const noexcept {
        static SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer> nullValue;
        auto&& pController = GetFrameBufferController();
        return pController ? pController->GetFramebuffer() : nullValue;
    }

    uint8_t FrameBufferPassData::GetLayersCount() const noexcept {
        auto&& pController = GetFrameBufferController();
        return pController ? pController->GetLayersCount() : 1;
    }

    const Pipeline::Ptr& FrameBufferPassData::GetPipeline() const noexcept {
        static Pipeline::Ptr nullValue;
        return m_renderTechnique ? m_renderTechnique->GetPipeline() : nullValue;
    }

    const FrameBufferController::Ptr& FrameBufferPassData::GetFrameBufferController() const noexcept {
        if (m_frameBufferName.Empty()) {
            SR_ERROR("FrameBufferPassData::GetFrameBufferController() : frame buffer name is empty!");
            return m_frameBufferController;
        }

        if (m_frameBufferController) {
            return m_frameBufferController;
        }
        if (m_renderTechnique) {
            m_frameBufferController = m_renderTechnique->GetFrameBufferController(m_frameBufferName);
        }
        if (!m_frameBufferController) {
            SR_ERROR("FrameBufferPassData::GetFrameBufferController() : failed to find frame buffer controller!\n\tName: {}", m_frameBufferName);
        }
        return m_frameBufferController;
    }
}
