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

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            SR_LOG("DebugWireframeMesh::Calculate() : calculating \"" + GetMeshIdentifier() + "\"...");
        }

        if (!CalculateVBO<Vertices::VertexType::SimpleVertex, Vertices::SimpleVertex>([this]() {
            return Vertices::CastVertices<Vertices::SimpleVertex>(GetVertices());
        })) {
            return false;
        }

        return IndexedMesh::Calculate();
    }

    const SR_HTYPES_NS::FastMemoryArray<uint32_t>& DebugWireframeMesh::GetIndices() const {
        return GetRawMesh()->GetIndices(GetMeshId());
    }

    bool DebugWireframeMesh::OnResourceReloaded(const SR_UTILS_NS::IResource* pResource) {
        bool changed = Mesh::OnResourceReloaded(pResource);
        if (GetRawMesh().Get() == pResource) {
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

    std::string DebugWireframeMesh::GetMeshIdentifier() const {
        if (auto&& pRawMesh = GetRawMesh()) {
            return SR_FORMAT("{}|{}|{}", pRawMesh->GetResourceId().c_str(), GetMeshId(), pRawMesh->GetReloadCount());
        }

        return Super::GetMeshIdentifier();
    }

    void DebugWireframeMesh::SetMatrix(const SR_MATH_NS::Matrix4x4& matrix) {
        m_modelMatrix = matrix;
        Super::SetMatrix(matrix);
    }

    void DebugWireframeMesh::OnRawMeshChanged() {
        IRawMeshHolder::OnRawMeshChanged();
        ReRegisterMesh();
    }
}