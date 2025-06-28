//
// Created by Monika on 05.04.2022.
//

#include <Graphics/Types/Geometry/ProceduralMesh.h>

#include <Codegen/ProceduralMesh.generated.hpp>

namespace SR_GTYPES_NS {
    void ProceduralMesh::SetVertices(const std::vector<Vertices::StaticMeshVertex>& vertices) {
        m_indices.clear();
        m_vertices.clear();

        m_countVertices = 0;
        m_countIndices = 0;

        SetDirtyMesh();

        std::unordered_map<Vertices::StaticMeshVertex, uint32_t> uniqueVertices;

        for (const auto& vertex : vertices) {
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back(vertex);
            }

            m_indices.push_back(uniqueVertices[vertex]);
        }

        m_countVertices = m_vertices.size();
        m_countIndices = m_indices.size();
    }

    bool ProceduralMesh::Calculate()  {
        SR_TRACY_ZONE;

        if (IsCalculated()) {
            return true;
        }

        FreeVideoMemory();

        if (!IsCalculatable()) {
            return false;
        }

        if (!CalculateVBO<Vertices::VertexType::StaticMeshVertex>(m_vertices)) {
            return false;
        }

        return Super::Calculate();
    }

    const SR_HTYPES_NS::FastMemoryArray<uint32_t>& ProceduralMesh::GetIndices() const {
        return m_indices;
    }

    bool ProceduralMesh::IsCalculatable() const {
        return m_countVertices > 0;
    }

    void ProceduralMesh::SwapIndexedVertices(SR_HTYPES_NS::FastMemoryArray<Vertices::StaticMeshVertex>& vertices) {
        std::swap(m_vertices, vertices);
        m_countVertices = static_cast<uint32_t>(m_vertices.size());
        SetDirtyMesh();
    }

    void ProceduralMesh::SwapIndices(SR_HTYPES_NS::FastMemoryArray<uint32_t>& indices) {
        std::swap(m_indices, indices);
        m_countIndices = static_cast<uint32_t>(m_indices.size());
        SetDirtyMesh();
    }

    void ProceduralMesh::SetIndexedVertices(void *pData, uint64_t count) {
        SR_TRACY_ZONE;

        if (!pData || count == 0) {
            m_vertices.clear();
        }
        else {
            m_vertices.resize((m_countVertices = count));
            memcpy(m_vertices.data(), pData, count * sizeof(Vertices::StaticMeshVertex));
        }
        m_countVertices = static_cast<uint32_t>(m_vertices.size());
        SetDirtyMesh();
    }

    void ProceduralMesh::SetIndices(void *pData, uint64_t count) {
        SR_TRACY_ZONE;

        if (!pData || count == 0) {
            m_indices.clear();
        }
        else {
            m_indices.resize((m_countIndices = count));
            memcpy(m_indices.data(), pData, count * sizeof(uint32_t));
        }
        m_countIndices = static_cast<uint32_t>(m_indices.size());
        SetDirtyMesh();
    }

    void ProceduralMesh::SetDirtyMesh() {
        m_isCalculated = false;
        MarkMaterialDirty();
        ReRegisterMesh();

        if (auto&& renderScene = TryGetRenderScene()) {
            renderScene->SetDirty();
        }
    }

    void ProceduralMesh::UseMaterial() {
        Super::UseMaterial();
        UseModelMatrix();
    }

    void ProceduralMesh::UseModelMatrix() {
        Super::UseModelMatrix();
        GetRenderContext()->GetCurrentShader()->SetMat4(SHADER_MODEL_MATRIX, GetMatrix());
    }
}
