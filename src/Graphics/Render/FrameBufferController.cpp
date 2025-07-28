//
// Created by Monika on 04.02.2024.
//

#include <Graphics/Render/FrameBufferController.h>
#include <Graphics/Types/Framebuffer.h>

#include <Codegen/FrameBufferController.generated.hpp>

namespace SR_GRAPH_NS {
    FrameBufferController::FrameBufferController()
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    FrameBufferController::~FrameBufferController() {
        if (m_framebuffer) {
            m_framebuffer->RemoveUsePoint();
            m_framebuffer = nullptr;
        }
    }

    void FrameBufferController::OnResize(const SR_MATH_NS::UVector2& size) {
        if (m_dynamicResizing && m_framebuffer) {
            m_framebuffer->SetSize(SR_MATH_NS::IVector2(
                    static_cast<int32_t>(static_cast<SR_MATH_NS::Unit>(size.x) * m_preScale.x),
                    static_cast<int32_t>(static_cast<SR_MATH_NS::Unit>(size.y) * m_preScale.y)
            ));
        }
    }

    bool FrameBufferController::InitializeFramebuffer(RenderContext* pContext) {
        /// fix zero size
        if (m_size.x == 0) {
            m_size.x = static_cast<int32_t>(pContext->GetWindowSize().x);
        }

        if (m_size.y == 0) {
            m_size.y = static_cast<int32_t>(pContext->GetWindowSize().y);
        }

        /// pre scale size
        SR_MATH_NS::IVector2 size = {
                static_cast<int32_t>(static_cast<SR_MATH_NS::Unit>(m_size.x) * m_preScale.x),
                static_cast<int32_t>(static_cast<SR_MATH_NS::Unit>(m_size.y) * m_preScale.y),
        };

        SRAssert(!m_framebuffer);

        /// initialize framebuffer
        if ((m_framebuffer = SR_GTYPES_NS::Framebuffer::Create(m_colorFormats, m_depthFormat, size))) {
            m_framebuffer->SetLayersCount(m_layersCount);
            m_framebuffer->SetSampleCount(m_samples);
            m_framebuffer->SetDepthEnabled(m_depthEnabled);
            m_framebuffer->SetDepthAspect(m_depthAspect);
            m_framebuffer->SetFeatures(m_features);
            m_framebuffer->AddUsePoint();
        }
        else {
            SR_ERROR("FrameBufferController::Init() : failed to create framebuffer!");
            return false;
        }

        if (m_framebuffer) {
            m_framebuffer->RegisterGraphicsResource();
        }

        return true;
    }
}