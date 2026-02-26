//
// Created by Monika on 20.07.2023.
//

#include <Graphics/Pipeline/FrameBufferQueue.h>
#include <Graphics/Types/Framebuffer.h>

namespace SR_GRAPH_NS {
    const FrameBufferQueue::Queues& FrameBufferQueue::GetQueues() const {
        return m_levels;
    }

    void FrameBufferQueue::Clear() {
        m_levels.clear();
    }

    void FrameBufferQueue::AddQueue(FrameBufferQueue::FrameBuffer pFrameBuffer, uint32_t queueIndex) {
        if (!pFrameBuffer) {
            SRHalt("FrameBufferQueue::AddQueue() : invalid framebuffer!");
            return;
        }

        if (m_levels.size() <= queueIndex) {
            m_levels.resize(queueIndex + 1);
        }
        m_levels[queueIndex].emplace_back(pFrameBuffer);
    }
}
