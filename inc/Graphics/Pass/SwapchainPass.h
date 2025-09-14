//
// Created by Monika on 22.07.2022.
//

#ifndef SR_ENGINE_GRAPHICS_SWAPCHAIN_PASS_H
#define SR_ENGINE_GRAPHICS_SWAPCHAIN_PASS_H

#include <Graphics/Pass/GroupPass.h>

#include <Utils/Math/Vector3.h>

namespace SR_GRAPH_NS {
    class SwapchainPass : public GroupPass {
        SR_CLASS()
    public:
        bool Render() override;

        SR_NODISCARD float_t GetClearDepth() const { return m_depth; }
        SR_NODISCARD SR_MATH_NS::FColor GetClearColor() const { return m_color; }

    private:
        /// @property @drag(0.01f)
        float_t m_depth = 1.f;
        /// @property @drag(0.01f)
        SR_MATH_NS::FColor m_color;

    };
}

#endif //SR_ENGINE_GRAPHICS_SWAPCHAIN_PASS_H
