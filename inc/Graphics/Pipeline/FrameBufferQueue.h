//
// Created by Monika on 20.07.2023.
//

#ifndef SR_ENGINE_FRAMEBUFFERQUEUE_H
#define SR_ENGINE_FRAMEBUFFERQUEUE_H

#include <Graphics/macros.h>

#include <Utils/Debug.h>
#include <Utils/Types/ArrayVector.h>

namespace SR_GTYPES_NS {
    class Framebuffer;
}

namespace SR_GRAPH_NS {
    class FrameBufferQueue {
        using FrameBuffer = SR_GTYPES_NS::Framebuffer*;
        using Layer = uint32_t;
        using Queues = std::vector<SR_HTYPES_NS::ArrayVector<FrameBuffer , 64>>;

    public:
        void AddQueue(FrameBuffer pFrameBuffer, uint32_t queueIndex);

        void Clear();

        SR_NODISCARD const Queues& GetQueues() const;

    private:
        Queues m_levels;

    };
}

#endif //SR_ENGINE_FRAMEBUFFERQUEUE_H
