//
// Created by Monika on 22.07.2022.
//

#include <Graphics/Pass/SwapchainPass.h>
#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS {
    SR_REGISTER_RENDER_PASS(SwapchainPass)

    bool SwapchainPass::Load(const SR_XML_NS::Node &passNode) {
        m_depth = passNode.TryGetAttribute("Depth").ToFloat(1.f);

        m_color.r = passNode.TryGetAttribute("R").ToFloat(0.f);
        m_color.g = passNode.TryGetAttribute("G").ToFloat(0.f);
        m_color.b = passNode.TryGetAttribute("B").ToFloat(0.f);
        m_color.a = passNode.TryGetAttribute("A").ToFloat(0.f);

        return GroupPass::Load(passNode);
    }

    bool SwapchainPass::Render() {
        SR_TRACY_ZONE_N("Swapchain pass");

        auto&& pipeline = GetContext()->GetPipeline();

        pipeline->SetCurrentFrameBuffer(nullptr);

        for (uint8_t i = 0; i < pipeline->GetBuildIterationsCount(); ++i) {
            pipeline->SetBuildIteration(i);

            pipeline->BindFrameBuffer(nullptr);
            pipeline->ClearBuffers(m_color.r, m_color.g, m_color.b, m_color.a, m_depth, 1);

            pipeline->BeginCmdBuffer();
            {
                pipeline->BeginRender();

                pipeline->SetViewport();
                pipeline->SetScissor();

                GroupPass::Render();

                pipeline->EndRender();
            }
            pipeline->EndCmdBuffer();
        }

        return true;
    }

    void SwapchainPass::Update() {
        GroupPass::Update();
    }

    void SwapchainPass::InitNode() {
        IExecutableNode::InitNode();

        AddInputData<SR_SRLM_NS::DataTypeFlow>();

        AddInputData<SR_SRLM_NS::DataTypeFloat>(SR_HASH_STR_REGISTER("Depth"));
        AddInputData<SR_SRLM_NS::DataTypeStruct>(SR_HASH_STR_REGISTER("FVector4"));

        AddOutputData<SR_SRLM_NS::DataTypeFlow>();
    }
}
