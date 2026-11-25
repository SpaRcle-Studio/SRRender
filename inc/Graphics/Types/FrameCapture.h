//
// Created by Codex on 25.11.2026.
//

#ifndef SR_ENGINE_GRAPHICS_TYPES_FRAME_CAPTURE_H
#define SR_ENGINE_GRAPHICS_TYPES_FRAME_CAPTURE_H

#include <Graphics/stdInclude.h>

#include <Utils/ECS/Component.h>

namespace SR_GRAPH_NS {
    class Pipeline;
}

namespace SR_GTYPES_NS {
    /// @category(Render)
    class FrameCapture : public SR_UTILS_NS::Component {
        SR_CLASS()
    public:
        ~FrameCapture() override;
        /// @method
        bool Request();
        /// @method @evaluate
        bool IsReady() const;
        /// @method
        SR_UTILS_NS::String FetchPng(uint32_t width, uint32_t height);
        /// @method
        void Cancel();

    private:
        SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::Pipeline> m_pPipeline;
        uint64_t m_request = SR_ID_INVALID;
        uint32_t m_width = 0;
        uint32_t m_height = 0;
    };
}

#endif // SR_ENGINE_GRAPHICS_TYPES_FRAME_CAPTURE_H
