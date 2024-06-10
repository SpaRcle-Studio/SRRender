//
// Created by Monika on 29.10.2021.
//

#include <Graphics/Types/Geometry/DebugWireframeMesh.h>
#include <Utils/Types/RawMesh.h>

namespace SR_GTYPES_NS {
    DebugWireframeMesh::DebugWireframeMesh()
        : Super(MeshType::Wireframe)
    { }

    void DebugWireframeMesh::Draw() {
        SR_TRACY_ZONE;

        if ((!IsCalculated() && !Calculate()) || m_hasErrors) {
            return;
        }

        if (m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
            m_virtualUBO = m_uboManager.AllocateUBO(m_virtualUBO);
            if (m_virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                m_hasErrors = true;
                return;
            }

            m_virtualDescriptor = m_descriptorManager.AllocateDescriptorSet(m_virtualDescriptor);
        }

        m_pipeline->BindVBO(m_VBO);
        m_pipeline->BindIBO(m_IBO);
        m_uboManager.BindUBO(m_virtualUBO);

        const auto result = m_descriptorManager.Bind(m_virtualDescriptor);

        if (m_pipeline->GetCurrentBuildIteration() == 0) {
            if (result == DescriptorManager::BindResult::Duplicated || m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
                UseSamplers();
                MarkUniformsDirty();
                m_descriptorManager.Flush();
            }
        }

        if (result != DescriptorManager::BindResult::Failed) SR_UNLIKELY_ATTRIBUTE {
            m_pipeline->DrawIndices(m_countIndices);
        }

        m_dirtyMaterial = false;
    }

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

    void DebugWireframeMesh::SetMatrix(const SR_MATH_NS::Matrix4x4& matrix4X4) {
        m_modelMatrix = matrix4X4;
        MarkUniformsDirty();
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

    const SR_MATH_NS::Matrix4x4 &DebugWireframeMesh::GetModelMatrix() const {
        return m_modelMatrix;
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