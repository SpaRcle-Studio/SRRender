//
// Created by Monika on 22.07.2022.
//

#include <Graphics/Pass/FramebufferPass.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS {
    SR_REGISTER_RENDER_PASS(FramebufferPass)
    SR_REGISTER_RENDER_PASS(ClearBuffersPass)

    bool FramebufferPass::Load(const SR_XML_NS::Node &passNode) {
        LoadFramebufferSettings(passNode);

        bool result = GroupPass::Load(passNode.TryGetNode("Passes"));

        SetName(passNode.TryGetAttribute("Name").ToString(passNode.Name()));

        return result;
    }

    bool FramebufferPass::PreRender() {
        GroupPass::PreRender();
        return false;
    }

    bool FramebufferPass::Render() {
        auto&& pFrameBuffer = GetFramebuffer();

        if (!pFrameBuffer) {
            m_isFrameBufferRendered = false;
            return false;
        }

        if (!pFrameBuffer->Bind()) {
            m_isFrameBufferRendered = false;
            return false;
        }

        if (!pFrameBuffer->BeginCmdBuffer(GetClearColors(), GetClearDepth())) {
            m_isFrameBufferRendered = false;
            return false;
        }

        pFrameBuffer->SetViewportScissor();

        if (pFrameBuffer->BeginRender()) {
            GroupPass::Render();
            pFrameBuffer->EndRender();
            pFrameBuffer->EndCmdBuffer();
        }

        GetPassPipeline()->SetCurrentFrameBuffer(nullptr);

        m_isFrameBufferRendered = true;

        /// Независимо от того, отрисовали мы что-то в кадровый буффер или нет,
        /// все равно возвращаем false (hasDrawData), так как технически, кадровый буффер
        /// не несет данных для рендера.
        return false;
    }

    bool FramebufferPass::PostRender() {
        GroupPass::PostRender();
        return false;
    }

    void FramebufferPass::OnResize(const SR_MATH_NS::UVector2 &size) {
        GroupPass::OnResize(size);
    }

    bool FramebufferPass::Init() {
        return GroupPass::Init();
    }

    void FramebufferPass::Update() {
        auto&& pFrameBuffer = GetFramebuffer();
        if (!pFrameBuffer || pFrameBuffer->IsDirty()) {
            return;
        }

        if (!m_isFrameBufferRendered) {
            return;
        }

        GetPassPipeline()->SetCurrentFrameBuffer(pFrameBuffer);

        GroupPass::Update();

        GetPassPipeline()->SetCurrentFrameBuffer(nullptr);
    }

    std::vector<SR_GTYPES_NS::Framebuffer*> FramebufferPass::GetFrameBuffers() const {
        if (!GetFramebuffer()) {
            return std::vector<SR_GTYPES_NS::Framebuffer*>(); /// NOLINT
        }
        return { GetFramebuffer() };
    }

    IRenderTechnique* FramebufferPass::GetFrameBufferRenderTechnique() const {
        return GetTechnique();
    }

    /// ----------------------------------------------------------------------------------------------------------------

    bool ClearBuffersPass::Render() {
        GetPassPipeline()->EndRender();

        auto&& pFrameBufferPass = dynamic_cast<FramebufferPass*>(GetParent());

        if (m_clearColor && pFrameBufferPass) {
            GetPassPipeline()->ClearColorBuffer(pFrameBufferPass->GetClearColors());
        }

        if (m_clearDepth) {
            GetPassPipeline()->ClearDepthBuffer(1.f);
        }

        GetPassPipeline()->BeginRender();
        return Super::Render();
    }

    bool ClearBuffersPass::Load(const SR_XML_NS::Node& passNode) {
        m_clearDepth = passNode.TryGetAttribute("ClearDepth").ToBool(true);
        m_clearColor = passNode.TryGetAttribute("ClearColor").ToBool(true);
        return Super::Load(passNode);
    }
}