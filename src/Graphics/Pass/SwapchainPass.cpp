//
// Created by Monika on 22.07.2022.
//

#include <Graphics/Pass/SwapchainPass.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Codegen/SwapchainPass.generated.hpp>

namespace SR_GRAPH_NS {
    bool SwapchainPass::Render() {
        SR_TRACY_ZONE_N("Swapchain pass");

        auto&& pPipeline = GetPipeline();

        pPipeline->SetCurrentFrameBuffer(nullptr);
        pPipeline->BindFrameBuffer(nullptr);

        pPipeline->ClearBuffers(m_color.r, m_color.g, m_color.b, m_color.a, m_depth, 1);

        pPipeline->BeginCmdBuffer();
        {
            pPipeline->BeginRender();

            pPipeline->SetViewport();
            pPipeline->SetScissor();

            GroupPass::Render();

            pPipeline->EndRender();
        }
        pPipeline->EndCmdBuffer();

        return true;
    }
}
