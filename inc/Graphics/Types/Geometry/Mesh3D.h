//
// Created by Nikita on 01.06.2021.
//

#ifndef SR_ENGINE_GRAPHICS_MESH_3D_H
#define SR_ENGINE_GRAPHICS_MESH_3D_H

#include <Utils/Types/IRawMeshHolder.h>
#include <Graphics/Types/Geometry/MeshComponent.h>

namespace SR_GTYPES_NS {
    class Mesh3D final : public IndexedMeshComponent, public SR_HTYPES_NS::IRawMeshHolder {
        using Super = IndexedMeshComponent;
        SR_REGISTER_NEW_COMPONENT(Mesh3D, 1002);
    public:
        Mesh3D();

    public:
        typedef Vertices::StaticMeshVertex VertexType;

    public:
        SR_NODISCARD bool InitializeEntity() noexcept override;

        void UseMaterial() override;
        void UseModelMatrix() override;

        void OnRawMeshChanged() override;
        bool OnResourceReloaded(SR_UTILS_NS::IResource* pResource) override;

        SR_NODISCARD bool IsCalculatable() const override;
        SR_NODISCARD std::vector<uint32_t> GetIndices() const override;
        SR_NODISCARD std::string GetMeshIdentifier() const override;
        SR_NODISCARD FrustumCullingType GetFrustumCullingType() const override { return m_frustumCullingType; }

    private:
        bool Calculate() override;

    private:
        FrustumCullingType m_frustumCullingType = FrustumCullingType::Sphere;

    };
}

#endif //SR_ENGINE_GRAPHICS_MESH_3D_H
