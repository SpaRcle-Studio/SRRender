//
// Created by Monika on 22.07.2022.
//

#ifndef SR_ENGINE_GRAPHICS_FRAME_BUFFER_PASS_H
#define SR_ENGINE_GRAPHICS_FRAME_BUFFER_PASS_H

#include <Graphics/Pass/GroupPass.h>
#include <Graphics/Pass/Data/FrameBufferPassData.h>

namespace SR_GTYPES_NS {
    class Framebuffer;
}

namespace SR_GRAPH_NS {
    class FrameBufferPass : public GroupPass {
        using Super = GroupPass;
        SR_CLASS()
    public:
        bool Render() override;
        void Update() override;

    public:
        SR_NODISCARD const FrameBufferPassData::ClearColors& GetClearColors() const noexcept;
        SR_NODISCARD const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Framebuffer>& GetFrameBuffer() const noexcept;
        SR_NODISCARD FrameBufferPassData& GetFrameBufferPassData() noexcept { return m_data; }
        SR_NODISCARD const FrameBufferPassData& GetFrameBufferPassData() const noexcept { return m_data; }

        void SetRenderTechnique(IRenderTechnique* pRenderTechnique) override;

    private:
        /// @property @noHeader
        FrameBufferPassData m_data;

    };

    class ClearBuffersPass : public BasePass {
        SR_CLASS()
        using Super = BasePass;
    public:
        bool Render() override;

    private:
        /// @property
        bool m_clearDepth = true;
        /// @property
        bool m_clearColor = true;

    };

    class ClearDepthAttachmentPass : public BasePass {
        SR_CLASS()
        using Super = BasePass;
    public:
        bool Render() override;

    private:
        /// @property
        float_t m_depth = 1.f;

    };
}

#endif //SR_ENGINE_GRAPHICS_FRAME_BUFFER_PASS_H
