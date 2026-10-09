//
// Created by Monika on 05.04.2022.
//

#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Geometry/ProceduralMesh.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/FileSystem/FileSystem.h>

#include <Codegen/ProceduralMesh.generated.hpp>

namespace SR_GTYPES_NS {
    const SR_HTYPES_NS::FastMemoryArray<uint32_t>& ProceduralMesh::GetIndices() const {
        return m_indices;
    }

    const SR_UTILS_NS::VertexDataBuffer& ProceduralMesh::GetVertices() const {
        if (!m_vertices) {
            static SR_UTILS_NS::VertexDataBuffer empty;
            return empty;
        }
        return *m_vertices;
    }

    bool ProceduralMesh::IsCalculatable() const {
        return m_countVertices > 0;
    }

    void ProceduralMesh::SwapIndices(SR_HTYPES_NS::FastMemoryArray<uint32_t>& indices) {
        std::swap(m_indices, indices);
        m_countIndices = static_cast<uint32_t>(m_indices.size());
        SetDirtyMesh(false);
    }

    void ProceduralMesh::SetIndexedVertices(const SR_UTILS_NS::VertexDataBuffer& vertices) {
        SR_TRACY_ZONE;

        if (!m_vertices) {
            m_vertices = new SR_UTILS_NS::VertexDataBuffer();
        }
        m_vertices->CopyFrom(vertices);
        m_countVertices = vertices.GetVertexCount();

        const bool layoutChanged = !GetVertexLayoutDescription().Compare(vertices.GetLayout());
        if (layoutChanged) {
            SetVertexLayoutDescription(vertices.GetLayout());
        }
        SetDirtyMesh(layoutChanged);
    }

    void ProceduralMesh::SetIndices(void* pData, uint64_t count) {
        SR_TRACY_ZONE;

        if (!pData || count == 0) {
            m_indices.clear();
        }
        else {
            m_indices.resize((m_countIndices = count));
            memcpy(m_indices.data(), pData, count * sizeof(uint32_t));
        }
        m_countIndices = static_cast<uint32_t>(m_indices.size());
        SetDirtyMesh(false);
    }

    void ProceduralMesh::SetDirtyMesh(bool layoutChanged) {
        m_isCalculated = false;
        MarkMaterialDirty();

        /// VBO в ключе очереди рендера. Если буферы переиспользуются (и layout тот же), он не меняется -
        /// перерегистрация не нужна, достаточно пересобрать командные буферы
        if (layoutChanged || m_VBO == SR_ID_INVALID || m_IBO == SR_ID_INVALID) {
            ReRegisterRenderObject();
        }

        if (auto&& renderScene = TryGetRenderScene()) {
            renderScene->SetDirty();
        }
    }

    bool ProceduralMesh::Calculate() {
        SR_TRACY_ZONE;

        if (IsCalculated()) {
            return true;
        }

        if (!m_vertices || !IsCalculatable() || m_indices.empty()) {
            FreeVideoMemory();
            return false;
        }

        m_isUniqueMesh = true;
        m_countIndices = static_cast<uint32_t>(m_indices.size());
        m_countVertices = static_cast<uint32_t>(m_vertices->GetVertexCount());

        const uint64_t vertexSize = m_vertices->GetDataSize();
        const uint64_t indexSize = m_indices.size() * sizeof(uint32_t);

        auto&& pPipeline = GetPipeline();

        const bool canReuse = m_VBO != SR_ID_INVALID && m_IBO != SR_ID_INVALID && vertexSize <= m_VBOCapacity && indexSize <= m_IBOCapacity;
        if (canReuse && pPipeline->UpdateVBO(m_VBO, m_vertices->GetRawData(), vertexSize) && pPipeline->UpdateIBO(m_IBO, m_indices.data(), indexSize)) {
            /// Mesh::Calculate - без освобождения UBO и дескрипторов, они остаются валидными
            return Mesh::Calculate();
        }

        /// Буферы не помещают данные - выделяются заново. Если VBO изменится, объект перерегистрируется
        const int32_t oldVBO = m_VBO;
        FreeVideoMemory();

        if (!AllocateBuffers()) {
            return false;
        }

        if (m_VBO != oldVBO && IsRenderObjectRegistered()) {
            ReRegisterRenderObject();
        }

        return Mesh::Calculate();
    }

    bool ProceduralMesh::AllocateBuffers() {
        SR_TRACY_ZONE;

        auto&& pPipeline = GetPipeline();

        /// Запас ёмкости: меш процедурный и часто перестраивается с близким размером
        const uint64_t vertexSize = m_vertices->GetDataSize();
        const uint64_t indexCount = m_indices.size();
        const uint64_t stride = std::max<uint64_t>(m_vertices->GetLayout().GetStride(), 1);
        const uint64_t vertexCapacity = (vertexSize + vertexSize / 2 + stride - 1) / stride * stride;
        const uint64_t indexCapacity = indexCount + indexCount / 2;

        /// Данные пишутся при выделении, хвост буфера остаётся неинициализированным - он не рисуется
        static SR_THREAD_LOCAL SR_HTYPES_NS::FastMemoryArray<uint8_t> staging;
        staging.resize(std::max(vertexCapacity, indexCapacity * sizeof(uint32_t)));

        std::memcpy(staging.data(), m_vertices->GetRawData(), vertexSize);
        if (m_VBO = pPipeline->AllocateVBO(SR_ID_INVALID, vertexCapacity, staging.data()); m_VBO == SR_ID_INVALID) {
            SR_ERROR("ProceduralMesh::AllocateBuffers() : failed to allocate VBO!");
            m_hasErrors = true;
            return false;
        }
        m_VBOCapacity = vertexCapacity;

        std::memcpy(staging.data(), m_indices.data(), indexCount * sizeof(uint32_t));
        if (m_IBO = pPipeline->AllocateIBO(staging.data(), sizeof(uint32_t), indexCapacity, m_VBO); m_IBO == SR_ID_INVALID) {
            SR_ERROR("ProceduralMesh::AllocateBuffers() : failed to allocate IBO!");
            m_hasErrors = true;
            return false;
        }
        m_IBOCapacity = indexCapacity * sizeof(uint32_t);

        return true;
    }

    void ProceduralMesh::FreeVideoMemory() {
        Super::FreeVideoMemory();
        m_VBOCapacity = 0;
        m_IBOCapacity = 0;
    }

    void ProceduralMesh::UseMaterial(SR_GTYPES_NS::Shader& shader) {
        Super::UseMaterial(shader);
        UseModelMatrix(shader);
    }

    void ProceduralMesh::UseModelMatrix(SR_GTYPES_NS::Shader& shader) {
        Super::UseModelMatrix(shader);
        shader.SetMat4(SHADER_MODEL_MATRIX, GetMatrix());
    }

    void ProceduralMesh::UseSSBO() {
        Super::UseSSBO();
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

        std::string content;
        content += "# Exported IndexedMesh\n";
        //content += "o " + GetMeshIdentifier() + "\n";

        SRHalt("ProceduralMesh::Export() : not implemented yet!");
        //for (uint64_t i = 0; i < GetVerticesCount(); ++i) {
        //    const auto& vertex = m_vertices[i];
        //    content += "v " + std::to_string(vertex.pos.x) + " " +
        //               std::to_string(vertex.pos.y) + " " +
        //               std::to_string(vertex.pos.z) + "\n";
        //}

        //for (uint64_t i = 0; i < GetVerticesCount(); ++i) {
        //    const auto& vertex = m_vertices[i];
        //    content += "vn " + std::to_string(vertex.norm.x) + " " +
        //               std::to_string(vertex.norm.y) + " " +
        //               std::to_string(vertex.norm.z) + "\n";
        //}

        //for (uint64_t i = 0; i < GetIndicesCount() / 3; ++i) {
        //    content += "f " + std::to_string(m_indices[i * 3] + 1) + " " +
        //               std::to_string(m_indices[i * 3 + 1] + 1) + " " +
        //               std::to_string(m_indices[i * 3 + 2] + 1) + "\n";
        //}

        if (!SR_UTILS_NS::FileSystem::WriteToFile(path.ToStringRef(), content)) {
            SR_ERROR("ProceduralMesh::Export() : failed to write to file! Path: {}", path.ToString());
            return false;
        }

        return true;
    }
}
