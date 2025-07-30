//
// Created by Monika on 21.01.2023.
//

#include <Graphics/Pass/Data/FrameBufferPassData.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/FrameBufferController.h>
#include <Graphics/Types/Framebuffer.h>

#include <Codegen/FrameBufferPassData.generated.hpp>

namespace SR_GRAPH_NS {
    FrameBufferPassData::FrameBufferPassData()
        : Super()
    { }

    /*void FrameBufferPassData::LoadFramebufferSettings(const SR_XML_NS::Node& passNode) {
        auto&& settingsNode = passNode.TryGetNode("FramebufferSettings");
        if (!settingsNode) {
            return;
        }

        m_isDirectional = settingsNode.TryGetAttribute("Directional").ToBool(false);

        if (!m_isDirectional) {
            m_frameBufferName = settingsNode.GetAttribute("Name").ToString();
            m_frameBufferController = GetFrameBufferRenderTechnique()->GetFrameBufferController(m_frameBufferName);
            if (!m_frameBufferController) {
                SR_ERROR("FrameBufferPassData::LoadFramebufferSettings() : failed to find frame buffer controller!\n\tName: " + m_frameBufferName.ToStringRef());
            }
        }

        for (auto&& subNode : settingsNode.GetNodes()) {
            /// color layers
            if (subNode.NameView() == "Layer") {
                SR_MATH_NS::FColor clearColor;

                clearColor.r = subNode.TryGetAttribute("R").ToFloat(0.f);
                clearColor.g = subNode.TryGetAttribute("G").ToFloat(0.f);
                clearColor.b = subNode.TryGetAttribute("B").ToFloat(0.f);
                clearColor.a = subNode.TryGetAttribute("A").ToFloat(1.f);

                m_clearColors.emplace_back(clearColor);
            }
                /// depth layer
            else if (subNode.NameView() == "Depth") {
                if (subNode.HasAttribute("ClearValue")) {
                    m_depth = subNode.GetAttribute("ClearValue").ToFloat(1.f);
                }
                else {
                    m_depth = std::nullopt;
                }
            }
        }
    }*/

    bool FrameBufferPassData::RenderFrameBuffer(const FBRenderCallback& callback) {
        m_isFrameBufferRendered = false;

        if (IsDirectional()) {
            GetPipeline()->SetCurrentFrameBuffer(nullptr);
            if (GetLayersCount() != 1) {
                SR_ERROR("FrameBufferPassData::RenderFrameBuffer() : directional frame buffer must have only one layer!\n\tName: {}", m_frameBufferName);
                return false;
            }
            m_isFrameBufferRendered = callback();
            return m_isFrameBufferRendered;
        }

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

        auto&& pPipeline = GetPipeline();

        if (GetLayersCount() > 1) {
            return RenderFrameBuffer(callback, GetLayersCount());
        }

        if (pFrameBuffer->Bind()) {
            for (uint32_t frame = 0; frame < pPipeline->GetBuildIterationsCount(); ++frame) {
                pFrameBuffer->BeginCmdBuffer(frame, m_clearColors, m_depth);
                {
                    pFrameBuffer->BeginRender();
                    pFrameBuffer->SetViewportScissor();

                    callback();
                    m_isFrameBufferRendered = true;

                    pFrameBuffer->EndRender();
                }
                pFrameBuffer->EndCmdBuffer();
            }
        }

        GetPipeline()->SetCurrentFrameBuffer(nullptr);

        return IsDirectional();
    }

    bool FrameBufferPassData::RenderFrameBuffer(const FBRenderCallback& callback, uint8_t layers) {
        auto&& pFrameBuffer = GetFramebuffer();
        auto&& pPipeline = GetPipeline();

        /// установим кадровый буфер, чтобы BeginCmdBuffer понимал какие значение для очистки ставить
        pPipeline->SetCurrentFrameBuffer(const_cast<Pipeline::FramebufferPtr>(pFrameBuffer.Get()));

        for (uint32_t frame = 0; frame < pPipeline->GetBuildIterationsCount(); ++frame) {
            pFrameBuffer->BeginCmdBuffer(frame, m_clearColors, m_depth);
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

            pFrameBuffer->EndCmdBuffer();
        }

        GetPipeline()->SetCurrentFrameBuffer(nullptr);

        return IsDirectional();
    }

    void FrameBufferPassData::UpdateFrameBuffer(const FBUpdateCallback& callback) {
        if (!m_isFrameBufferRendered) {
            return;
        }

        auto&& pFrameBuffer = GetFramebuffer();

        if (!IsDirectional() && (!pFrameBuffer || pFrameBuffer->IsDirty())) {
            return;
        }

        GetPipeline()->SetCurrentFrameBuffer(const_cast<Pipeline::FramebufferPtr>(pFrameBuffer.Get()));

        for (uint32_t i = 0; i < GetLayersCount(); ++i) {
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
