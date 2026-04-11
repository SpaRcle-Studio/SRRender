//
// Created by Monika on 05.04.2022.
//

#ifndef SR_ENGINE_PROCEDURALMESH_H
#define SR_ENGINE_PROCEDURALMESH_H

#include <Graphics/Types/Geometry/IndexedMesh.h>

#include <Utils/Types/FastMemoryArray.h>

namespace SR_GTYPES_NS {
    class ProceduralMesh final : public IndexedMesh {
        using Super = IndexedMesh;
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<ProceduralMesh>;

    public:
        ProceduralMesh() = default;

    public:
        void SwapIndices(SR_HTYPES_NS::FastMemoryArray<uint32_t>& indices);
        void SetIndices(void* pData, uint64_t count);

        void SetIndexedVertices(const SR_UTILS_NS::VertexDataBuffer& vertices);

        void UseMaterial(SR_GTYPES_NS::Shader& shader) override;
        void UseModelMatrix(SR_GTYPES_NS::Shader& shader) override;

        SR_NODISCARD bool IsCalculatable() const override;
        SR_NODISCARD bool IsSupportVBO() const override;

        bool Export(const SR_UTILS_NS::Path& path) const;

    private:
        void SetDirtyMesh();
        void UseSSBO() override;

        SR_NODISCARD const SR_HTYPES_NS::FastMemoryArray<uint32_t>& GetIndices() const override;
        SR_NODISCARD const SR_UTILS_NS::VertexDataBuffer& GetVertices() const override;

    private:
        SR_UTILS_NS::VertexDataBuffer::Ptr m_vertices;
        SR_HTYPES_NS::FastMemoryArray<uint32_t> m_indices;

    };
}

#endif //SR_ENGINE_PROCEDURALMESH_H
