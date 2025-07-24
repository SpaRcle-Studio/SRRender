//
// Created by Monika on 05.04.2022.
//

#include <Graphics/Types/Geometry/ProceduralMesh.h>

#include <Codegen/ProceduralMesh.generated.hpp>

namespace SR_GTYPES_NS {
    void ProceduralMesh::SetVertices(const std::vector<Vertices::StaticMeshVertex>& vertices) {
        SR_TRACY_ZONE;

        SRAssert(!m_useSSBOInsteadOfVertices);

        m_indices.clear();
        m_vertices.clear();

        m_vertices.reserve(vertices.size());
        m_indices.reserve(vertices.size());

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

    void ProceduralMesh::SetVertices(const SR_HTYPES_NS::FastMemoryArray<Vertices::StaticMeshVertex>& vertices) {
        SR_TRACY_ZONE;

        if (m_useSSBOInsteadOfVertices) {
            SRHalt("ProceduralMesh::SetVertices() : cannot set vertices when using SSBO!");
            return;
        }

        m_indices.clear();
        m_vertices.clear();

        m_vertices.reserve(vertices.size());
        m_indices.reserve(vertices.size());

        m_countVertices = 0;
        m_countIndices = 0;

        SetDirtyMesh();

        std::unordered_map<Vertices::StaticMeshVertex, uint32_t> uniqueVertices;

        for (uint32_t i = 0; i < vertices.size(); ++i) {
            auto& vertex = vertices[i];
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

        const uint32_t size = m_countVertices * sizeof(Vertices::StaticMeshVertexAligned);
        int32_t oldSSBO = SR_ID_INVALID;
        if (m_ssboSize >= size && m_useSSBOInsteadOfVertices) {
            std::swap(m_ssbo, oldSSBO);
        }

        FreeVMemory();

        if (!IsCalculatable()) {
            return false;
        }

        if (m_useSSBOInsteadOfVertices) {
            if (oldSSBO == SR_ID_INVALID) {
                SRAssert(m_ssbo == SR_ID_INVALID);
                m_ssbo = GetPipeline()->AllocateSSBO(size, SSBOUsage::CPUToGPU);
                m_ssboSize = size;
            }
            else {
                m_ssbo = oldSSBO;
            }

            GetPipeline()->UpdateSSBO(m_ssbo, (void*)m_verticesAligned.data(), size);

            m_isCalculated = true;
            /// чтобы в случае перезагрузки обновить все связанные данные
            MarkMaterialDirty();
        }
        else {
            if (!CalculateVBO<Vertices::VertexType::StaticMeshVertex>(m_vertices)) {
                return false;
            }

            return Super::Calculate();
        }

        return true;
    }

    const SR_HTYPES_NS::FastMemoryArray<uint32_t>& ProceduralMesh::GetIndices() const {
        static SR_HTYPES_NS::FastMemoryArray<uint32_t> empty;
        return m_useSSBOInsteadOfVertices ? empty : m_indices;
    }

    bool ProceduralMesh::IsCalculatable() const {
        return m_countVertices > 0;
    }

    void ProceduralMesh::SwapIndexedVertices(SR_HTYPES_NS::FastMemoryArray<Vertices::StaticMeshVertexAligned>& vertices) {
        SRAssert(m_useSSBOInsteadOfVertices);
        std::swap(m_verticesAligned, vertices);
        m_countVertices = static_cast<uint32_t>(m_verticesAligned.size());
        m_countIndices = m_countVertices;
        SetDirtyMesh();
    }

    void ProceduralMesh::SwapIndexedVertices(SR_HTYPES_NS::FastMemoryArray<Vertices::StaticMeshVertex>& vertices) {
        SRAssert(!m_useSSBOInsteadOfVertices);
        std::swap(m_vertices, vertices);
        m_countVertices = static_cast<uint32_t>(m_vertices.size());
        SetDirtyMesh();
    }

    void ProceduralMesh::SwapIndices(SR_HTYPES_NS::FastMemoryArray<uint32_t>& indices) {
        SRAssert(!m_useSSBOInsteadOfVertices);
        std::swap(m_indices, indices);
        m_countIndices = static_cast<uint32_t>(m_indices.size());
        SetDirtyMesh();
    }

    void ProceduralMesh::SetIndexedVertices(void *pData, uint64_t count) {
        SR_TRACY_ZONE;

        SRAssert(!m_useSSBOInsteadOfVertices);

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

        SRAssert(!m_useSSBOInsteadOfVertices);

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
        GetPipeline()->GetCurrentShader()->SetMat4(SHADER_MODEL_MATRIX, GetMatrix());
    }

    bool ProceduralMesh::IsSupportVBO() const {
        return !m_useSSBOInsteadOfVertices;
    }

    void ProceduralMesh::UseSSBO() {
        if (m_useSSBOInsteadOfVertices && m_ssbo != SR_ID_INVALID) {
            GetPipeline()->GetCurrentShader()->BindSSBO("ssboVertices", m_ssbo);
        }
        Super::UseSSBO();
    }

    void ProceduralMesh::FreeVMemory() {
        if (m_ssbo != SR_ID_INVALID) {
            GetPipeline()->FreeSSBO(&m_ssbo);
        }
        Super::FreeVMemory();
    }

    bool ProceduralMesh::Export(const SR_UTILS_NS::Path& path) const {
        SR_TRACY_ZONE;

        if (path.empty()) {
            SR_ERROR("ProceduralMesh::Export() : path is empty!");
            return false;
        }

        if (path.GetExtensionView() != "obj") {
            SR_ERROR("ProceduralMesh::Export() : only .obj format is supported!");
            return false;
        }

        if (!path.CreateIfNotExists()) {
            SR_ERROR("ProceduralMesh::Export() : failed to create directory for export! Path: {}", path.ToString());
            return false;
        }

        std::string content;
        content += "# Exported IndexedMesh\n";
        content += "o " + GetMeshIdentifier() + "\n";

        for (uint64_t i = 0; i < GetVerticesCount(); ++i) {
            const auto& vertex = m_vertices[i];
            content += "v " + std::to_string(vertex.pos.x) + " " +
                       std::to_string(vertex.pos.y) + " " +
                       std::to_string(vertex.pos.z) + "\n";
        }

        for (uint64_t i = 0; i < GetVerticesCount(); ++i) {
            const auto& vertex = m_vertices[i];
            content += "vn " + std::to_string(vertex.norm.x) + " " +
                       std::to_string(vertex.norm.y) + " " +
                       std::to_string(vertex.norm.z) + "\n";
        }

        for (uint64_t i = 0; i < GetIndicesCount() / 3; ++i) {
            content += "f " + std::to_string(m_indices[i * 3] + 1) + " " +
                       std::to_string(m_indices[i * 3 + 1] + 1) + " " +
                       std::to_string(m_indices[i * 3 + 2] + 1) + "\n";
        }

        if (path.IsFile()) {
            SR_PLATFORM_NS::Delete(path);
        }

        if (!SR_UTILS_NS::FileSystem::WriteToFile(path.ToStringRef(), content)) {
            SR_ERROR("ProceduralMesh::Export() : failed to write to file! Path: {}", path.ToString());
            return false;
        }

        return true;
    }
}
