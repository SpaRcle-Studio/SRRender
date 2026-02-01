//
// Created by Monika on 20.07.2023.
//

#include <Graphics/Pipeline/FrameBufferQueue.h>
#include <Graphics/Types/Framebuffer.h>

namespace SR_GRAPH_NS {
    void FrameBufferQueue::AddFrameBuffer(FrameBufferQueue::FrameBuffer pFrameBuffer, uint32_t layer) {
        SR_TRACY_ZONE;

        auto&& pIt = std::ranges::find_if(m_used, [pFrameBuffer](const auto& pair) { return pair.fbo == pFrameBuffer; });

        if (pIt == m_used.end()) {
            auto&& info = m_used.emplace_back();
            info.fbo = pFrameBuffer;
            info.layers.emplace_back(layer);
        }
        else {
            auto&& info = *pIt;
            if (std::ranges::find(info.layers, layer) == info.layers.end()) {
                info.layers.emplace_back(layer);
            }
            else {
                SRHalt("FrameBufferQueue::AddFrameBuffer() : framebuffer for layer already exists!");
            }
        }
    }

    const FrameBufferQueue::Queues& FrameBufferQueue::GetQueues() const {
        return m_levels;
    }

    bool FrameBufferQueue::Contains(FrameBufferQueue::FrameBuffer pFrameBuffer, uint32_t layer) {
        if (IsAllowMultiFrameBuffers()) {
            return false;
        }

        auto&& pIt = std::ranges::find_if(m_used, [pFrameBuffer](const auto& pair) { return pair.fbo == pFrameBuffer; });
        if (pIt != m_used.end()) {
            return std::ranges::find(pIt->layers, layer) != pIt->layers.end();
        }

        return false;
    }

    bool FrameBufferQueue::Contains(FrameBufferQueue::FrameBuffer pFrameBuffer) {
        if (IsAllowMultiFrameBuffers()) {
            return false;
        }
        auto&& pIt = std::ranges::find_if(m_used, [pFrameBuffer](const auto& pair) { return pair.fbo == pFrameBuffer; });
        return pIt != m_used.end();
    }

    void FrameBufferQueue::Clear() {
        m_used.clear();
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

    bool FrameBufferQueue::IsAllowMultiFrameBuffers() const {
        return true;
    }
}
