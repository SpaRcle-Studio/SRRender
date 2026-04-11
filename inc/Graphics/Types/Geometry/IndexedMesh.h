//
// Created by Monika on 30.10.2021.
//

#ifndef SR_ENGINE_GRAPHICS_INDEXEDMESH_H
#define SR_ENGINE_GRAPHICS_INDEXEDMESH_H

#include <Graphics/Types/Mesh.h>

#include <Utils/Types/FastMemoryArray.h>

namespace SR_GTYPES_NS {
    /// @abstract
    class IndexedMesh : public Mesh {
        using Super = Mesh;
        SR_CLASS()
    public:
        IndexedMesh() = default;
        ~IndexedMesh() override;

    public:
        SR_NODISCARD int32_t GetIBO() override;
        SR_NODISCARD int32_t GetVBO() override;

        SR_NODISCARD uint32_t GetVerticesCount() const override { return m_countVertices; }
        SR_NODISCARD uint32_t GetIndicesCount() const override { return m_countIndices; }

        SR_NODISCARD virtual const SR_HTYPES_NS::FastMemoryArray<uint32_t>& GetIndices() const {
            static SR_HTYPES_NS::FastMemoryArray<uint32_t> empty;
            return empty;
        }

        SR_NODISCARD virtual const SR_UTILS_NS::VertexDataBuffer& GetVertices() const {
            static SR_UTILS_NS::VertexDataBuffer empty;
            return empty;
        }

        SR_NODISCARD const SR_UTILS_NS::VertexLayoutDescription& GetShaderVertexLayoutDescription() const noexcept override;

        SR_NODISCARD bool IsSupportVBO() const override { return true; }

        SR_NODISCARD FrustumCullingType GetFrustumCullingType() const noexcept override { return m_frustumCullingType; }
        void SetFrustumCullingType(FrustumCullingType type) override { m_frustumCullingType = type; }

        bool Calculate() override;

        bool CalculateIBO();
        bool CalculateVBO();

        void FreeVMemory() override;

        bool FreeVBO();
        bool FreeIBO();

    public:
        /// @property
        FrustumCullingType m_frustumCullingType = FrustumCullingType::AABB;

    protected:
        int32_t m_IBO = SR_ID_INVALID;
        int32_t m_VBO = SR_ID_INVALID;
        uint32_t m_countIndices = 0;
        uint32_t m_countVertices = 0;
        bool m_isUniqueMesh = false;

    };
}

#endif //SR_ENGINE_GRAPHICS_INDEXEDMESH_H
