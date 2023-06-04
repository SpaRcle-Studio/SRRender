//
// Created by Monika on 19.09.2022.
//

#include <Graphics/Pass/DebugPass.h>
#include <Graphics/Types/Material.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Geometry/IndexedMesh.h>
#include <Graphics/Pipeline/IShaderProgram.h>

namespace SR_GRAPH_NS {
    SR_REGISTER_RENDER_PASS(DebugPass)

    DebugPass::DebugPass(RenderTechnique *pTechnique, BasePass* pParent)
        : Super(pTechnique, pParent)
    { }

    MeshClusterTypeFlag DebugPass::GetClusterType() const noexcept {
        return static_cast<MeshClusterTypeFlag>(MeshClusterType::Debug);
    }

    void DebugPass::UseUniforms(SR_GTYPES_NS::Shader* pShader) {
        pShader->SetMat4(SHADER_VIEW_MATRIX, m_camera->GetViewTranslateRef());
        pShader->SetMat4(SHADER_PROJECTION_MATRIX, m_camera->GetProjectionRef());
        pShader->SetMat4(SHADER_ORTHOGONAL_MATRIX, m_camera->GetOrthogonalRef());
        pShader->SetVec3(SHADER_VIEW_DIRECTION, m_camera->GetViewDirection());
        Super::UseUniforms(pShader);
    }
}