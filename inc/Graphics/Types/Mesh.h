//
// Created by Nikita on 17.11.2020.
//

#ifndef SR_ENGINE_GRAPHICS_MESH_H
#define SR_ENGINE_GRAPHICS_MESH_H

#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Types/IRenderComponent.h>

#include <Utils/Math/AABB.h>
#include <Utils/UI/MaskInfo.h>
#include <Utils/Common/Enumerations.h>
#include <Utils/Types/Function.h>
#include <Utils/Types/SortedVector.h>

namespace SR_UTILS_NS {
    class IResource;
}

namespace SR_HTYPES_NS {
    class RawMesh;
}

namespace SR_GRAPH_NS {
    class RenderScene;
    class RenderContext;
    class DescriptorManager;
}

namespace SR_GRAPH_NS {
    class BaseMaterial;
}

namespace SR_GTYPES_NS {
    class Shader;

    class Mesh : public SR_GTYPES_NS::IRenderComponent {
        SR_CLASS()
        using Super = SR_GTYPES_NS::IRenderComponent;
    public:
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<RenderScene>;
        using ShaderPtr = SR_HTYPES_NS::SharedPtr<Shader>;
        using MaterialPtr = SR_HTYPES_NS::SharedPtr<BaseMaterial>;
        using Ptr = SR_HTYPES_NS::SharedPtr<Mesh>;

        using RenderQueues = MeshRenderQueues;

    public:
        Mesh();
        ~Mesh() override;

    public:
        static std::vector<Mesh::Ptr> Load(const SR_UTILS_NS::Path& path, MeshType type);
        static Mesh::Ptr TryLoad(SR_HTYPES_NS::RawMesh* pRawMesh, MeshType type, uint32_t id);
        static Mesh::Ptr TryLoad(const SR_UTILS_NS::Path& path, MeshType type, uint32_t id);
        static Mesh::Ptr Load(const SR_UTILS_NS::Path& path, MeshType type, uint32_t id);
        static Mesh::Ptr Load(const SR_UTILS_NS::Path& path, MeshType type, SR_UTILS_NS::StringAtom name);

    public:
        void OnDestroy() override;
        void OnMatrixDirty() override;
        void OnMaskDirty() override;
        void OnLayerChanged() override;
        void OnPriorityChanged() override;
        void OnEnable() override;
        void OnDisable() override;

        SR_NODISCARD virtual int32_t GetIBO() { return SR_ID_INVALID; }
        SR_NODISCARD virtual int32_t GetVBO() { return SR_ID_INVALID; }

        SR_NODISCARD virtual bool IsCalculatable() const;
        SR_NODISCARD virtual bool IsUniqueMesh() const { return false; }

        SR_NODISCARD bool IsActive() const noexcept override;
        SR_NODISCARD virtual SR_FORCE_INLINE bool IsFlatMesh() const noexcept { return false; }
        SR_NODISCARD virtual std::string GetMeshIdentifier() const;
        SR_NODISCARD virtual int64_t GetSortingPriority() const;
        SR_NODISCARD virtual bool HasSortingPriority() const;
        SR_NODISCARD virtual SR_UTILS_NS::StringAtom GetMeshLayer() const;
        SR_NODISCARD virtual bool IsSupportVBO() const = 0;
        SR_NODISCARD virtual uint32_t GetIndicesCount() const = 0;
        SR_NODISCARD virtual uint32_t GetVerticesCount() const { return 0; }
        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }

        SR_NODISCARD Pipeline* GetPipeline() const noexcept { return m_pipeline; }
        SR_NODISCARD const MaterialPtr& GetMaterial() const noexcept { return m_material; }
        SR_NODISCARD MaterialPtr& GetMaterial() noexcept { return m_material; }
        SR_NODISCARD int32_t GetVirtualUBO() const { return m_virtualUBO; }
        SR_NODISCARD virtual MeshType GetMeshTypeImpl() const noexcept = 0;
        SR_NODISCARD MeshType GetMeshType() const noexcept;
        SR_NODISCARD bool IsWaitReRegister() const noexcept { return m_isWaitReRegister; }
        SR_NODISCARD bool IsMeshRegistered() const noexcept { return m_registrationInfo.has_value(); }
        SR_NODISCARD const MeshRegistrationInfo& GetMeshRegistrationInfo() const noexcept { return m_registrationInfo.value(); }
        SR_NODISCARD RenderQueues& GetRenderQueues() noexcept { return m_renderQueues; }
        SR_NODISCARD FrustumCullingType GetFrustumCullingType() const noexcept;
        SR_NODISCARD bool IsCalculated() const noexcept { return m_isCalculated; }
        SR_NODISCARD bool IsFrustumCullingSupported() const noexcept;
        SR_NODISCARD const SR_MATH_NS::AABB& GetAABB() const;
        SR_NODISCARD const SR_UTILS_NS::UI::MaskInfo& GetMaskInfo() const;

        void SetFrustumCullingType(FrustumCullingType type) { m_frustumCullingType = type; }

        void SetMeshRegistrationInfo(const std::optional<MeshRegistrationInfo>& info) { m_registrationInfo = info; }
        void SetPipeline(Pipeline* pPipeline) { m_pipeline = pPipeline; }

        virtual void SetMatrix(const SR_MATH_NS::Matrix4x4& matrix);

        SR_NODISCARD virtual const SR_MATH_NS::Matrix4x4& GetMatrix() const;

        virtual bool OnResourceReloaded(const SR_UTILS_NS::IResource* pResource);
        virtual bool BindMesh();

        virtual void Draw();

        virtual void UseMaterial(SR_GTYPES_NS::Shader& shader);
        virtual void UseSamplers(SR_GTYPES_NS::Shader& shader);
        virtual void UseModelMatrix(SR_GTYPES_NS::Shader& shader) { }
        virtual void UseSSBO() { }

        void OnReRegistered();
        void MarkUniformsDirty();
        void MarkMaterialDirty();
        bool DestroyMesh();
        void ReRegisterMesh();
        void UnRegisterMesh();

        void SetMaterial(const MaterialPtr& pMaterial);
        void SetMaterial(const SR_UTILS_NS::Path& path);

        void SetErrorsClean() { m_hasErrors = false; }

    protected:
        virtual void FreeVMemory();
        virtual bool Calculate();

    protected:
        RenderQueues m_renderQueues;

        Memory::UBOManager& m_uboManager;
        Pipeline* m_pipeline = nullptr;
        SR_GRAPH_NS::DescriptorManager& m_descriptorManager;

        int32_t m_virtualUBO = SR_ID_INVALID;
        int32_t m_virtualDescriptor = SR_ID_INVALID;

    protected:
        /// @virtualProperty(meshType) @getter(GetMeshType) @readOnly @dontSave
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(verticesCount) @getter(GetVerticesCount) @readOnly @dontSave
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(indicesCount) @getter(GetIndicesCount) @readOnly @dontSave
        SR_VIRTUAL_PROPERTY
        /// @property @setter(SetMaterial) @getter(GetMaterial) @inspector(MaterialPropertyDrawer)
        MaterialPtr m_material;
        /// @property @propertyCondition(This.IsFrustumCullingSupported())
        FrustumCullingType m_frustumCullingType = FrustumCullingType::AABB;
        /// @property @readOnly @dontSave
        bool m_isWaitReRegister = false;
        /// @virtualProperty(isRegistered) @getter(IsMeshRegistered) @readOnly @dontSave
        SR_VIRTUAL_PROPERTY
        /// @property @readOnly @dontSave
        bool m_isCalculated = false;
        /// @property @readOnly @dontSave
        bool m_hasErrors = false;
        /// @property @readOnly @dontSave
        bool m_dirtyMaterial = false;

    private:
        mutable MeshType m_meshTypeCache = MeshType::Unknown;
        mutable SR_UTILS_NS::UI::MaskInfo m_maskInfo;
        mutable SR_MATH_NS::AABB m_aabb;
        mutable bool m_isAABBDirty = true;
        mutable bool m_isMaskDirty = true;
        bool m_isDestroyingState = false;
        std::optional<MeshRegistrationInfo> m_registrationInfo;
        uint32_t m_materialRegisterId = SR_ID_INVALID;

    };

    constexpr static size_t SIZE_OF_MESH_CLASS = sizeof(Mesh);
}

#endif //SR_ENGINE_GRAPHICS_MESH_H
