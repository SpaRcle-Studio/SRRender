//
// Created by Monika on 05.04.2022.
//

#ifndef SR_ENGINE_PROCEDURALMESH_H
#define SR_ENGINE_PROCEDURALMESH_H

#include <Graphics/Types/Geometry/MeshComponent.h>

namespace SR_GTYPES_NS {
    class ProceduralMesh final : public IndexedMesh {
        using Super = IndexedMesh;
        SR_REGISTER_NEW_COMPONENT(ProceduralMesh, 1003);
    public:
        ProceduralMesh() = default;

    public:
        typedef Vertices::StaticMeshVertex VertexType;

    public:
        SR_NODISCARD MeshType GetMeshType() const noexcept override { return MeshType::Procedural; }

        void SetIndexedVertices(void* pData, uint64_t count);
        void SetIndices(void* pData, uint64_t count);
        void SetVertices(const std::vector<Vertices::StaticMeshVertex>& vertices);

        void UseMaterial() override;
        void UseModelMatrix() override;

        SR_NODISCARD bool IsUniqueMesh() const override { return true; }

        SR_NODISCARD bool IsCalculatable() const override;

    private:
        bool Calculate() override;
        void Draw() override;
        void SetDirtyMesh();

        SR_NODISCARD std::vector<uint32_t> GetIndices() const override;

    private:
        std::vector<Vertices::StaticMeshVertex> m_vertices;
        std::vector<uint32_t> m_indices;

    };
}

#endif //SR_ENGINE_PROCEDURALMESH_H
