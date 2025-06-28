//
// Created by Nikita on 01.06.2021.
//

#ifndef SR_ENGINE_GRAPHICS_MESH_3D_H
#define SR_ENGINE_GRAPHICS_MESH_3D_H

#include <Utils/Types/IRawMeshHolder.h>

#include <Graphics/Types/Geometry/MeshComponent.h>

namespace SR_GTYPES_NS {
    /// @category(Render)
    class Mesh3D final : public IndexedMesh, public SR_HTYPES_NS::IRawMeshHolder {
        using Super = IndexedMesh;
        SR_CLASS()
    public:
        Mesh3D() = default;

    public:
        typedef Vertices::StaticMeshVertex VertexType;

    public:
        void UseMaterial() override;
        void UseModelMatrix() override;

        void OnRawMeshChanged() override;
        bool OnResourceReloaded(SR_UTILS_NS::IResource* pResource) override;

        SR_NODISCARD MeshType GetMeshType() const noexcept override { return MeshType::Static; }

        SR_NODISCARD bool IsCalculatable() const override;
        SR_NODISCARD const SR_HTYPES_NS::FastMemoryArray<uint32_t>& GetIndices() const override;
        SR_NODISCARD std::string GetMeshIdentifier() const override;

    private:
        bool Calculate() override;

    private:
        /// @virtualProperty(geometryName) @getter(GetGeometryName) @dontSave @readOnly
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(meshPath) @getter(GetMeshPath) @setter(SetRawMesh)
        /// @customArgs(pick: enabled, filter name: Meshes)
        /// @customArg(filter value: fbx,blend,obj,pmx,stl,dae)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(meshId) @getter(GetMeshId) @setter(SetMeshId)
        SR_VIRTUAL_PROPERTY

    };
}

#endif //SR_ENGINE_GRAPHICS_MESH_3D_H
