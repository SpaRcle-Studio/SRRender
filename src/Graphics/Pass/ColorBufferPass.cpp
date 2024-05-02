//
// Created by Monika on 10.10.2022.
//

#include <Graphics/Pass/ColorBufferPass.h>
#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS {
    SR_REGISTER_RENDER_PASS(ColorBufferPass)

    void ColorBufferPass::Update() {
        ResetColorIndex();
        Super::Update();
    }

    void ColorBufferPass::UseUniforms(ShaderUseInfo info, MeshPtr pMesh) {
        if (info.useMaterial) {
            pMesh->UseMaterial();
        }

        pMesh->UseModelMatrix();
        pMesh->UseOverrideUniforms();

        IncrementColorIndex();
        SetMeshIndex(pMesh);

        info.pShader->SetVec3(SHADER_COLOR_BUFFER_VALUE, GetMeshColor());
    }

    SR_GTYPES_NS::Framebuffer* ColorBufferPass::GetColorFrameBuffer() const noexcept {
        return GetFramebuffer();
    }

    void ColorBufferPass::UseConstants(ShaderUseInfo info) {
        Super::UseConstants(info);
        info.pShader->SetConstInt(SHADER_COLOR_BUFFER_MODE, 1);
    }

    bool ColorBufferPass::Render() {
        ClearTable();
        m_needUpdateUniforms = true;
        return OffScreenMeshDrawerPass::Render();
    }

    bool ColorBufferPass::IsNeedUpdate() const noexcept {
        if (m_needUpdateUniforms) {
            return true;
        }

        if (auto&& pStrategy = GetPassPipeline()->GetCurrentRenderStrategy()) {
            return pStrategy->IsUniformsDirty();
        }

        return true;
    }

    void ColorBufferPass::PostUpdate() {
        m_needUpdateUniforms = false;
        Super::PostUpdate();
    }

    bool ColorBufferPass::Load(const SR_XML_NS::Node& passNode) {
        SetColorMultiplier(passNode.TryGetAttribute("ColorMultiplier").ToInt(1));
        return Super::Load(passNode);
    }
}
