//
// Created by Monika on 19.09.2022.
//

#ifndef SR_ENGINE_GRAPHICS_MESH_COMPONENT_H
#define SR_ENGINE_GRAPHICS_MESH_COMPONENT_H

#include <Graphics/Types/Geometry/IndexedMesh.h>
#include <Graphics/Types/IRenderComponent.h>

namespace SR_GTYPES_NS {
    class IMeshComponent : public SR_GTYPES_NS::IRenderComponent {
        using Super = SR_GTYPES_NS::IRenderComponent;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<IMeshComponent>;

    protected:
        explicit IMeshComponent(Mesh* pMesh);

    public:
        SR_NODISCARD bool InitializeEntity() noexcept override;

        void OnDestroy() override;
        void OnMatrixDirty() override;

        void OnEnable() override;
        void OnDisable() override;

        void OnLayerChanged() override;
        void OnPriorityChanged() override;

        SR_NODISCARD bool ExecuteInEditMode() const override;
        SR_NODISCARD bool IsUpdatable() const noexcept override { return false; }

    protected:
        /// TODO: remove it
        std::string m_geometryName;

    private:
        Mesh* m_pInternal = nullptr;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class IndexedMeshComponent : public IMeshComponent, public IndexedMesh {
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<IndexedMeshComponent>;

    protected:
        explicit IndexedMeshComponent(MeshType type);

    public:
        SR_NODISCARD SR_FORCE_INLINE bool IsMeshActive() const noexcept override {
            return SR_UTILS_NS::Component::IsActive() && IndexedMesh::IsMeshActive();
        }

        SR_NODISCARD std::string GetGeometryName() const override { return m_geometryName; }
        void SetGeometryName(const std::string& name) override { m_geometryName = name; }

        SR_NODISCARD int64_t GetSortingPriority() const override;
        SR_NODISCARD bool HasSortingPriority() const override;
        SR_NODISCARD SR_UTILS_NS::StringAtom GetMeshLayer() const override;

        SR_NODISCARD const SR_MATH_NS::Matrix4x4& GetMatrix() const override;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class MeshComponent : public IMeshComponent, public Mesh {
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<MeshComponent>;

    protected:
        explicit MeshComponent(MeshType type);

    public:
        SR_NODISCARD SR_FORCE_INLINE bool IsMeshActive() const noexcept override {
            return SR_UTILS_NS::Component::IsActive() && Mesh::IsMeshActive();
        }

        SR_NODISCARD std::string GetGeometryName() const override { return m_geometryName; }
        void SetGeometryName(const std::string& name) override { m_geometryName = name; }
        const SR_MATH_NS::Matrix4x4& GetMatrix() const override;

        SR_NODISCARD int64_t GetSortingPriority() const override;
        SR_NODISCARD bool HasSortingPriority() const override;
        SR_NODISCARD SR_UTILS_NS::StringAtom GetMeshLayer() const override;

    };
}

#endif //SR_ENGINE_GRAPHICS_MESH_COMPONENT_H
