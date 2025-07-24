//
// Created by Nikita on 01.06.2021.
//

#ifndef SR_ENGINE_GRAPHICS_SKINNED_MESH_H
#define SR_ENGINE_GRAPHICS_SKINNED_MESH_H

#include <Utils/Types/IRawMeshHolder.h>
#include <Utils/ECS/EntityRef.h>

#include <Graphics/Types/Geometry/MeshComponent.h>
#include <Graphics/Animations/Skeleton.h>

namespace SR_GTYPES_NS {
    /// @category(Render)
    class SkinnedMesh final : public IndexedMesh, public SR_HTYPES_NS::IRawMeshHolder {
        SR_CLASS()
        using Super = IndexedMesh;
        using SkeletonRef = SR_UTILS_NS::EntityRef<SR_ANIMATIONS_NS::Skeleton>;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<SkinnedMesh>;

        SkinnedMesh();

    public:
        typedef Vertices::SkinnedMeshVertex VertexType;

    public:
        SR_NODISCARD MeshType GetMeshType() const noexcept override { return MeshType::Skinned; }

        void LateUpdate() override;
        void UseMaterial() override;
        void UseModelMatrix() override;

        SR_NODISCARD bool IsCalculatable() const override;
        SR_NODISCARD bool IsUpdatable() const noexcept override { return true; }
        SR_NODISCARD std::string GetMeshIdentifier() const override;
        SR_NODISCARD const SkeletonRef& GetSkeletonRef() const noexcept { return m_skeleton; }
        SR_NODISCARD SkeletonRef& GetSkeletonRef() noexcept { return m_skeleton; }

        void FreeVMemory() override;

        void UseSSBO() override;

    private:
        bool OnResourceReloaded(const SR_UTILS_NS::IResource* pResource) override;
        void OnRawMeshChanged() override;
        bool Calculate() override;

        void FreeSSBO();

        SR_NODISCARD const SR_HTYPES_NS::FastMemoryArray<uint32_t>& GetIndices() const override;

    private:
        bool m_skeletonIsBroken = false;
        int32_t m_ssboBones = SR_ID_INVALID;
        int32_t m_ssboOffsets = SR_ID_INVALID;

    private:
        /// @property
        SkeletonRef m_skeleton;

        /// @virtualProperty(geometryName) @getter(GetGeometryName) @dontSave @readOnly
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(meshPath) @getter(GetMeshPath) @setter(SetRawMesh)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(meshId) @getter(GetMeshId) @setter(SetMeshId)
        SR_VIRTUAL_PROPERTY

    };
}

#endif //SR_ENGINE_GRAPHICS_SKINNED_MESH_H
