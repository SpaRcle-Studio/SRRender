//
// Created by Monika on 10.10.2022.
//

#include <Graphics/Pass/ColorBufferPass.h>
#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS {
    SR_REGISTER_RENDER_PASS(ColorBufferPass)

    ColorBufferRenderQueue::ColorBufferRenderQueue(RenderStrategy* pStrategy, MeshDrawerPass* pDrawer)
        : Super(pStrategy, pDrawer)
    {
        m_customMeshDraw = true;
    }

    void ColorBufferRenderQueue::CustomDrawMesh(const MeshInfo& info) {
        auto pColorBuffer = static_cast<ColorBufferPass*>(GetMeshDrawerPass());
        pColorBuffer->IncrementColorIndex();
        pColorBuffer->SetMeshIndex(info.pMesh);
        info.shaderUseInfo.pShader->SetConstVec3(SHADER_COLOR_BUFFER_VALUE, pColorBuffer->GetMeshColor());
        info.pMesh->Draw();
    }

    void ColorBufferPass::UseUniforms(ShaderUseInfo info, MeshPtr pMesh) {
        if (info.useMaterialUniforms) {
            pMesh->UseMaterial();
        }
        pMesh->UseModelMatrix();
    }

    MeshDrawerPass::RenderQueuePtr ColorBufferPass::AllocateRenderQueue() {
        return GetRenderStrategy()->BuildQueue<ColorBufferRenderQueue, RenderQueue>(this);
    }

    SR_GTYPES_NS::Framebuffer* ColorBufferPass::GetColorFrameBuffer() const noexcept {
        return GetFramebuffer();
    }

    void ColorBufferPass::UseConstants(ShaderUseInfo info) {
        Super::UseConstants(info);
        info.pShader->SetConstInt(SHADER_COLOR_BUFFER_MODE, 1);
    }

    bool ColorBufferPass::Render() {
        ResetColorIndex();
        ClearTable();
        return OffScreenMeshDrawerPass::Render();
    }

    bool ColorBufferPass::Load(const SR_XML_NS::Node& passNode) {
        SetColorMultiplier(passNode.TryGetAttribute("ColorMultiplier").ToInt(1));
        return Super::Load(passNode);
    }
}
