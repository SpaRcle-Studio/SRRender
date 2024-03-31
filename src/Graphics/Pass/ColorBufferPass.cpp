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
        SetMeshIndex(pMesh, GetColorIndex());

        info.pShader->SetVec3(SHADER_COLOR_BUFFER_VALUE, GetMeshColor());
    }

    SR_GTYPES_NS::Framebuffer* ColorBufferPass::GetColorFrameBuffer() const noexcept {
        return GetFramebuffer();
    }

    void ColorBufferPass::UseConstants(ShaderUseInfo info) {
        Super::UseConstants(info);
        info.pShader->SetConstInt(SHADER_COLOR_BUFFER_MODE, 1);
    }
}
