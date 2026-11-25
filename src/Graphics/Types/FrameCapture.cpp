//
// Created by Codex on 25.11.2026.
//

#include <Graphics/Types/FrameCapture.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Loaders/TextureLoader.h>

#include <Utils/World/Scene.h>

#include <Codegen/FrameCapture.generated.hpp>

namespace SR_GTYPES_NS {
    FrameCapture::~FrameCapture() {
        Cancel();
    }

    void FrameCapture::Cancel() {
        if (m_pPipeline && m_request != SR_ID_INVALID) {
            m_pPipeline->ReleaseFrameCapture(m_request);
        }
        m_request = SR_ID_INVALID;
        m_pPipeline = nullptr;
    }

    bool FrameCapture::Request() {
        SR_TRACY_ZONE;
        Cancel();
        auto&& pScene = TryGetScene();
        auto&& pRender = pScene ? dynamic_cast<SR_GRAPH_NS::RenderScene*>(pScene->GetModule("Render")) : nullptr;
        auto pCamera = pRender ? pRender->GetMainCamera() : nullptr;
        auto pContext = pRender ? pRender->GetContext() : nullptr;
        if (!pCamera || !pContext) {
            return false;
        }
        m_pPipeline = pRender->GetPipeline();
        SR_HTYPES_NS::SharedPtr<Framebuffer> pFramebuffer;
        if (pCamera->IsEditorCamera()) {
            pFramebuffer = pContext->FindFramebuffer(pContext->GetSettings().editorSceneImageName, pCamera.Get());
            if (!pFramebuffer || pFramebuffer->IsFramebufferDead()) {
                Cancel();
                return false;
            }
            m_width = pFramebuffer->GetWidth();
            m_height = pFramebuffer->GetHeight();
        }
        else {
            const auto size = pContext->GetWindowSize();
            m_width = size.x;
            m_height = size.y;
        }
        if (m_width == 0 || m_height == 0 || m_width > 8192 || m_height > 8192) {
            Cancel();
            return false;
        }
        m_request = m_pPipeline->RequestFrameCapture(pFramebuffer, m_width, m_height);
        return m_request != SR_ID_INVALID;
    }

    bool FrameCapture::IsReady() const {
        return m_pPipeline && m_request != SR_ID_INVALID && m_pPipeline->IsFrameCaptureReady(m_request);
    }

    SR_UTILS_NS::String FrameCapture::FetchPng(uint32_t width, uint32_t height) {
        SR_TRACY_ZONE;
        if (!IsReady() || width == 0 || height == 0 || width > 8192 || height > 8192) {
            return {};
        }
        SR_UTILS_NS::Vector<SR_MATH_NS::FColor> pixels(m_width * m_height);
        if (!m_pPipeline->GetFrameCaptureResult(m_request, pixels.data())) {
            Cancel();
            return {};
        }
        SR_UTILS_NS::Vector<uint8_t> rgb(width * height * 3);
        // Center-crop to the requested aspect ratio, excluding letterbox bars.
        const uint32_t cropWidth = SR_MIN(m_width, static_cast<uint32_t>(static_cast<uint64_t>(m_height) * width / height));
        const uint32_t cropHeight = SR_MIN(m_height, static_cast<uint32_t>(static_cast<uint64_t>(m_width) * height / width));
        const auto channelByte = [](float_t value) -> uint8_t {
            return std::isfinite(value) ? static_cast<uint8_t>(SR_CLAMP(value, 0.f, 1.f) * 255.f) : 0;
        };
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const uint32_t sourceX = (m_width - cropWidth) / 2 + static_cast<uint64_t>(x) * cropWidth / width;
                const uint32_t sourceY = (m_height - cropHeight) / 2 + static_cast<uint64_t>(y) * cropHeight / height;
                const auto& color = pixels[sourceY * m_width + sourceX];
                rgb[(y * width + x) * 3] = channelByte(color.r);
                rgb[(y * width + x) * 3 + 1] = channelByte(color.g);
                rgb[(y * width + x) * 3 + 2] = channelByte(color.b);
            }
        }
        auto result = SR_GRAPH_NS::TextureLoader::EncodeRgbPng(rgb.data(), width, height);
        Cancel();
        return result;
    }
}
