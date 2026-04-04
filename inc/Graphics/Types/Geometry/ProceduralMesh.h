//
// Created by Monika on 05.04.2022.
//

#ifndef SR_ENGINE_PROCEDURALMESH_H
#define SR_ENGINE_PROCEDURALMESH_H

#include <Graphics/Types/Geometry/MeshComponent.h>

#include <Utils/Types/FastMemoryArray.h>

namespace SR_GTYPES_NS {
    class ProceduralMesh final : public IndexedMesh {
        using Super = IndexedMesh;
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<ProceduralMesh>;
        typedef Vertices::StaticMeshVertex VertexType;

    public:
        ProceduralMesh() = default;

    public:
        SR_NODISCARD MeshType GetMeshTypeImpl() const noexcept override { return MeshType::Procedural; }

        void SwapIndices(SR_HTYPES_NS::FastMemoryArray<uint32_t>& indices);
        void SetIndices(void* pData, uint64_t count);

        void SetIndexedVertices(void* pData, uint64_t count, Vertices::VertexType vertexType);

        void UseMaterial(SR_GTYPES_NS::Shader& shader) override;
        void UseModelMatrix(SR_GTYPES_NS::Shader& shader) override;

        SR_NODISCARD bool IsUniqueMesh() const override { return true; }
        SR_NODISCARD bool IsCalculatable() const override;
        SR_NODISCARD bool IsSupportVBO() const override;

        bool Export(const SR_UTILS_NS::Path& path) const;

    private:
        void FreeVMemory() override;
        bool Calculate() override;
        void SetDirtyMesh();
        void UseSSBO() override;

        SR_NODISCARD const SR_HTYPES_NS::FastMemoryArray<uint32_t>& GetIndices() const override;

    private:
        /// @property @onChanged(SetDirtyMesh)
        bool m_useSSBOInsteadOfVertices = false;

        int32_t m_ssbo = SR_ID_INVALID;
        uint32_t m_ssboSize = 0;

        SR_HTYPES_NS::FastMemoryArray<char*> m_verticesData;
        uint32_t m_countVertices = 0;
        Vertices::VertexType m_verticesType = Vertices::VertexType::Unknown;

        SR_HTYPES_NS::FastMemoryArray<uint32_t> m_indices;

    };
}

#endif //SR_ENGINE_PROCEDURALMESH_H
