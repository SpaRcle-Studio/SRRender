//
// Created by Monika on 21.01.2023.
//

#ifndef SR_ENGINE_IFRAME_BUFFER_PASS_H
#define SR_ENGINE_IFRAME_BUFFER_PASS_H

#include <Graphics/Pipeline/TextureHelper.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Render/FrameBufferController.h>
#include <Graphics/Memory/UBOManager.h>

namespace SR_GTYPES_NS {
    class Framebuffer;
}

namespace SR_GRAPH_NS {
    class IRenderTechnique;

    class FrameBufferPassData final : public SR_UTILS_NS::Serializable {
        SR_CLASS()
        using Super = SR_UTILS_NS::Serializable;
    public:
        using ClearColors = std::vector<SR_MATH_NS::FColor>;
        using FBRenderCallback = std::function<bool()>;
        using FBUpdateCallback = std::function<void()>;

    public:
        FrameBufferPassData();

    public:
        SR_NODISCARD const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>& GetFramebuffer() const noexcept;
        SR_NODISCARD bool IsFrameBufferRendered() const noexcept { return m_isFrameBufferRendered; }
        SR_NODISCARD const ClearColors& GetClearColors() const noexcept { return m_clearColors; }
        SR_NODISCARD ClearColors& GetClearColors() noexcept { return m_clearColors; }
        SR_NODISCARD std::optional<float_t> GetClearDepth() const noexcept { return m_depth; }
        SR_NODISCARD uint8_t GetLayersCount() const noexcept;

        bool RenderFrameBuffer(const FBRenderCallback& callback);
        void UpdateFrameBuffer(const FBUpdateCallback& callback);

        void SetRenderTechnique(IRenderTechnique* pRenderTechnique) noexcept { m_renderTechnique = pRenderTechnique; }
        void SetFrameBufferName(SR_UTILS_NS::StringAtom name) noexcept { m_frameBufferName = name; }
        void SetClearColors(const ClearColors& colors) { m_clearColors = colors; }
        void SetClearDepth(float_t depth) { m_depth = depth; }
        void SetClearDepth(std::optional<float_t> depth) { m_depth = depth; }

    private:
        bool RenderFrameBuffer(const FBRenderCallback& callback, uint8_t layers);

        SR_NODISCARD const Pipeline::Ptr& GetPipeline() const noexcept;
        SR_NODISCARD const FrameBufferController::Ptr& GetFrameBufferController() const noexcept;

    private:
        mutable FrameBufferController::Ptr m_frameBufferController;
        IRenderTechnique* m_renderTechnique = nullptr;
        bool m_isFrameBufferRendered = false;

        /// @property
        ClearColors m_clearColors;
        /// @property
        SR_UTILS_NS::StringAtom m_frameBufferName;
        /// @property
        std::optional<float_t> m_depth = 1.f;

    };
}

#endif //SR_ENGINE_IFRAMEBUFFERPASS_H
