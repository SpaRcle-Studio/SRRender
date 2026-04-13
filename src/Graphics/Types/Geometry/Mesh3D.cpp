//
// Created by Nikita on 02.06.2021.
//

#include <Utils/Types/RawMesh.h>
#include <Utils/Types/DataStorage.h>
#include <Utils/ECS/ComponentManager.h>

#include <Graphics/Types/Geometry/Mesh3D.h>
#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Utils/MeshUtils.h>

#include <Utils/FileSystem/PathDataAccessor.h>

#include <Codegen/Mesh3D.generated.hpp>

namespace SR_GTYPES_NS {
    const SR_HTYPES_NS::FastMemoryArray<uint32_t>& Mesh3D::GetIndices() const {
        SR_TRACY_ZONE;
        return GetRawMesh()->GetIndices(GetMeshId());
    }

    bool Mesh3D::IsCalculatable() const {
        return IsValidMeshId() && Super::IsCalculatable();
    }

    void Mesh3D::UseMaterial(SR_GTYPES_NS::Shader& shader) {
        Super::UseMaterial(shader);
        UseModelMatrix(shader);
    }

    void Mesh3D::UseModelMatrix(SR_GTYPES_NS::Shader& shader) {
        shader.SetMat4(SHADER_MODEL_MATRIX, GetMatrix());
    }

    void Mesh3D::OnRawMeshChanged() {
        IRawMeshHolder::OnRawMeshChanged();
        ReRegisterRenderObject();
        MarkMaterialDirty();
        m_isCalculated = false;
    }

    bool Mesh3D::OnResourceReloaded(SR_UTILS_NS::StringAtom resourceId) {
        bool changed = Mesh::OnResourceReloaded(resourceId);
        if (GetRawMesh() && GetRawMesh()->GetResourceId() == resourceId) {
            OnRawMeshChanged();
            return true;
        }
        return changed;
    }

    const SR_UTILS_NS::VertexDataBuffer& Mesh3D::GetVertices() const {
        SR_TRACY_ZONE;
        return GetRawMesh()->GetVertexBuffer(GetMeshId(), GetVertexLayoutDescription());
    }
}