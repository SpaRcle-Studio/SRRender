//
// Created by Igor on 27/11/2022.
//

#include <Graphics/Types/Geometry/SkinnedMesh.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/Common/Features.h>
#include <Utils/Types/RawMesh.h>
#include <Utils/FileSystem/PathDataAccessor.h>

#include <Codegen/SkinnedMesh.generated.hpp>

namespace SR_GTYPES_NS {
    const SR_HTYPES_NS::FastMemoryArray<uint32_t>& SkinnedMesh::GetIndices() const {
        return GetRawMesh()->GetIndices(GetMeshId());
    }

    bool SkinnedMesh::IsCalculatable() const {
        return IsValidMeshId() && Mesh::IsCalculatable();
    }

    void SkinnedMesh::UseMaterial(SR_GTYPES_NS::Shader& shader) {
        Super::UseMaterial(shader);
        UseModelMatrix(shader);
    }

    void SkinnedMesh::UseModelMatrix(SR_GTYPES_NS::Shader& shader) {
        shader.SetMat4(SHADER_MODEL_MATRIX, GetMatrix());
    }

    bool SkinnedMesh::OnResourceReloaded(SR_UTILS_NS::StringAtom resourceId) {
        bool changed = Mesh::OnResourceReloaded(resourceId);
        if (GetRawMesh() && GetRawMesh()->GetResourceId() == resourceId) {
            OnRawMeshChanged();
            return true;
        }
        return changed;
    }

    void SkinnedMesh::OnRawMeshChanged() {
        IRawMeshHolder::OnRawMeshChanged();

        ReRegisterMesh();

        MarkMaterialDirty();
        m_isCalculated = false;
    }

    void SkinnedMesh::UseSSBO() {
        SR_TRACY_ZONE;

        auto&& pSkeleton = m_skeleton.Get();
        if (!pSkeleton) {
            return Super::UseSSBO();
        }

        const int32_t bonesSSBO = pSkeleton->GetBonesSSBO();
        if (bonesSSBO == SR_ID_INVALID) {
            return Super::UseSSBO();
        }

        const int32_t offsetsSSBO = pSkeleton->GetOffsetsSSBO();

        GetPipeline()->GetCurrentShader()->BindSSBO("bones", bonesSSBO);

        if (offsetsSSBO != SR_ID_INVALID) {
            GetPipeline()->GetCurrentShader()->BindSSBO("offsets", offsetsSSBO);
        }
        else {
            GetPipeline()->GetCurrentShader()->BindSSBO("offsets", bonesSSBO);
        }

        Super::UseSSBO();
    }

    const SR_UTILS_NS::VertexDataBuffer& SkinnedMesh::GetVertices() const {
        return GetRawMesh()->GetVertexBuffer(GetMeshId(), GetVertexLayoutDescription());
    }
}
