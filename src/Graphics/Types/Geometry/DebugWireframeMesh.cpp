//
// Created by Monika on 29.10.2021.
//

#include <Graphics/Types/Geometry/DebugWireframeMesh.h>
#include <Graphics/Material/BaseMaterial.h>

#include <Utils/Types/RawMesh.h>

#include <Codegen/DebugWireframeMesh.generated.hpp>

namespace SR_GTYPES_NS {
    bool DebugWireframeMesh::Calculate() {
        if (IsCalculated()) {
            return true;
        }

        FreeVMemory();

        if (!IsCalculatable()) {
            return false;
        }

        return Super::Calculate();
    }

    const SR_HTYPES_NS::FastMemoryArray<uint32_t>& DebugWireframeMesh::GetIndices() const {
        return GetRawMesh()->GetIndices(GetMeshId());
    }

    bool DebugWireframeMesh::OnResourceReloaded(SR_UTILS_NS::StringAtom resourceId) {
        bool changed = Mesh::OnResourceReloaded(resourceId);
        if (GetRawMesh() && GetRawMesh()->GetResourceId() == resourceId) {
            OnRawMeshChanged();
            return true;
        }
        return changed;
    }

    void DebugWireframeMesh::UseMaterial(SR_GTYPES_NS::Shader& shader) {
        Mesh::UseMaterial(shader);
        static const uint64_t colorHashName = SR_UTILS_NS::StringAtom("color").GetHash();
        shader.SetMat4(SHADER_MODEL_MATRIX, GetMatrix());
        shader.SetVec4(colorHashName, m_color.Cast<float_t>().ToGLM());
    }

    void DebugWireframeMesh::SetColor(const SR_MATH_NS::FVector4& color) {
        m_color = color;
        MarkUniformsDirty();
    }

    void DebugWireframeMesh::SetMatrix(const SR_MATH_NS::Matrix4x4& matrix) {
        m_modelMatrix = matrix;
        Super::SetMatrix(matrix);
    }

    void DebugWireframeMesh::OnRawMeshChanged() {
        IRawMeshHolder::OnRawMeshChanged();
        ReRegisterMesh();
    }

    const SR_UTILS_NS::VertexDataBuffer& DebugWireframeMesh::GetVertices() const {
        return GetRawMesh()->GetVertexBuffer(GetMeshId(), GetVertexLayoutDescription());
    }
}