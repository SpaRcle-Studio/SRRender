//
// Created by Monika on 29.10.2021.
//

#include <Graphics/Types/Geometry/DebugWireframeMesh.h>
#include <Utils/Types/RawMesh.h>

namespace SR_GTYPES_NS {
    DebugWireframeMesh::DebugWireframeMesh()
        : Super(MeshType::Wireframe)
    { }

    bool DebugWireframeMesh::Calculate() {
        if (IsCalculated()) {
            return true;
        }

        FreeVideoMemory();

        if (!IsCalculatable()) {
            return false;
        }

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            SR_LOG("DebugWireframeMesh::Calculate() : calculating \"" + GetGeometryName() + "\"...");
        }

        if (!CalculateVBO<Vertices::VertexType::SimpleVertex, Vertices::SimpleVertex>([this]() {
            return Vertices::CastVertices<Vertices::SimpleVertex>(GetVertices());
        })) {
            return false;
        }

        return IndexedMesh::Calculate();
    }

    std::vector<uint32_t> DebugWireframeMesh::GetIndices() const {
        return GetRawMesh()->GetIndices(GetMeshId());
    }

    bool DebugWireframeMesh::OnResourceReloaded(SR_UTILS_NS::IResource* pResource) {
        bool changed = Mesh::OnResourceReloaded(pResource);
        if (GetRawMesh() == pResource) {
            OnRawMeshChanged();
            return true;
        }
        return changed;
    }

    void DebugWireframeMesh::UseMaterial() {
        Mesh::UseMaterial();
        static const uint64_t colorHashName = SR_UTILS_NS::StringAtom("color").GetHash();
        GetShader()->SetMat4(SHADER_MODEL_MATRIX, m_modelMatrix);
        GetShader()->SetVec4(colorHashName, m_color.Cast<float_t>().ToGLM());
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

    void DebugWireframeMesh::OnRawMeshChanged() {
        IRawMeshHolder::OnRawMeshChanged();
        ReRegisterMesh();
    }
}