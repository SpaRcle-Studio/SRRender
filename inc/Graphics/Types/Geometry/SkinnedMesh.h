//
// Created by Nikita on 01.06.2021.
//

#ifndef SR_ENGINE_GRAPHICS_SKINNED_MESH_H
#define SR_ENGINE_GRAPHICS_SKINNED_MESH_H

#include <Graphics/Types/Geometry/IndexedMesh.h>
#include <Graphics/Animations/Skeleton.h>

#include <Utils/Types/IRawMeshHolder.h>
#include <Utils/ECS/EntityRef.h>

namespace SR_GTYPES_NS {
    /// @category(Render)
    class SkinnedMesh final : public IndexedMesh, public SR_HTYPES_NS::IRawMeshHolder {
        SR_CLASS()
        using Super = IndexedMesh;
        using SkeletonRef = SR_UTILS_NS::EntityRef<SR_ANIMATIONS_NS::Skeleton>;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<SkinnedMesh>;

    public:
        void UseMaterial(SR_GTYPES_NS::Shader& shader) override;
        void UseModelMatrix(SR_GTYPES_NS::Shader& shader) override;

        SR_NODISCARD bool IsCalculatable() const override;
        SR_NODISCARD bool IsUpdatable() const noexcept override { return true; }
        SR_NODISCARD const SkeletonRef& GetSkeletonRef() const noexcept { return m_skeleton; }
        SR_NODISCARD SkeletonRef& GetSkeletonRef() noexcept { return m_skeleton; }

        void UseSSBO() override;

    private:
        bool OnResourceReloaded(SR_UTILS_NS::StringAtom resourceId) override;
        void OnRawMeshChanged() override;

        SR_NODISCARD const SR_HTYPES_NS::FastMemoryArray<uint32_t>& GetIndices() const override;
        SR_NODISCARD const SR_UTILS_NS::VertexDataBuffer& GetVertices() const override;

    private:
        /// @property
        SkeletonRef m_skeleton;

        /// @virtualProperty(geometryName) @getter(GetGeometryName) @dontSave @readOnly
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(meshPath) @getter(GetMeshPath) @setter(SetRawMesh)
        /// @customArgs(pick: enabled, filter name: Meshes, relative: resources)
        /// @customArg(filter value: fbx,blend,obj,pmx,stl,dae)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(meshId) @getter(GetMeshId) @setter(SetMeshId)
        SR_VIRTUAL_PROPERTY

    };
}

#endif //SR_ENGINE_GRAPHICS_SKINNED_MESH_H
