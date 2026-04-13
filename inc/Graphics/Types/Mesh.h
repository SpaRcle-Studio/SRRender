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
#include <Utils/Common/Vertices.h>
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

    namespace Details {
        struct MeshInternalData {
            SR_MATH_NS::AABB aabb;
            SR_UTILS_NS::VertexLayoutDescription vertexLayoutDescription;
            std::optional<MeshRegistrationInfo> registrationInfo;
            uint32_t materialRegisterId = SR_ID_INVALID;
            MeshRenderQueues renderQueues;
        };
    }

    /// @abstract
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
        static std::vector<Mesh::Ptr> Load(const SR_UTILS_NS::Path& path);
        static Mesh::Ptr TryLoad(SR_HTYPES_NS::RawMesh* pRawMesh, uint32_t id);
        static Mesh::Ptr TryLoad(const SR_UTILS_NS::Path& path, uint32_t id);
        static Mesh::Ptr Load(const SR_UTILS_NS::Path& path, uint32_t id);
        static Mesh::Ptr Load(const SR_UTILS_NS::Path& path, SR_UTILS_NS::StringAtom name);

    public:
        SR_NODISCARD virtual int32_t GetIBO() { return SR_ID_INVALID; }
        SR_NODISCARD virtual int32_t GetVBO() { return SR_ID_INVALID; }

        SR_NODISCARD virtual bool IsCalculatable() const;

        SR_NODISCARD bool IsActive() const noexcept override;
        SR_NODISCARD virtual int64_t GetSortingPriority() const;
        SR_NODISCARD virtual bool HasSortingPriority() const;
        SR_NODISCARD virtual SR_UTILS_NS::StringAtom GetMeshLayer() const;
        SR_NODISCARD virtual bool IsSupportVBO() const { return false; }
        SR_NODISCARD virtual uint32_t GetIndicesCount() const { return 0; }
        SR_NODISCARD virtual uint32_t GetVerticesCount() const { return 0; }
        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }

        SR_NODISCARD Pipeline* GetPipeline() const noexcept { return m_pipeline; }
        SR_NODISCARD const MaterialPtr& GetMaterial() const noexcept { return m_material; }
        SR_NODISCARD MaterialPtr& GetMaterial() noexcept { return m_material; }
        SR_NODISCARD int32_t GetVirtualUBO() const { return m_virtualUBO; }
        SR_NODISCARD bool IsWaitReRegister() const noexcept { return m_isWaitReRegister; }
        SR_NODISCARD bool IsMeshRegistered() const noexcept;
        SR_NODISCARD const MeshRegistrationInfo& GetMeshRegistrationInfo() const noexcept;
        SR_NODISCARD RenderQueues& GetRenderQueues() noexcept;
        SR_NODISCARD bool IsCalculated() const noexcept { return m_isCalculated; }
        SR_NODISCARD const SR_MATH_NS::AABB& GetAABB() const;
        SR_NODISCARD const SR_UTILS_NS::UI::MaskInfo& GetMaskInfo() const;
        SR_NODISCARD const SR_UTILS_NS::VertexLayoutDescription& GetVertexLayoutDescription() const noexcept;
        SR_NODISCARD virtual const SR_UTILS_NS::VertexLayoutDescription& GetShaderVertexLayoutDescription() const noexcept;

        SR_NODISCARD virtual FrustumCullingType GetFrustumCullingType() const noexcept { return FrustumCullingType::None; }
        virtual void SetFrustumCullingType(FrustumCullingType type) { }

        void SetVertexLayoutDescription(const SR_UTILS_NS::VertexLayoutDescription& description);

        void SetMeshRegistrationInfo(const std::optional<MeshRegistrationInfo>& info);
        void SetPipeline(Pipeline* pPipeline) { m_pipeline = pPipeline; }

        SR_NODISCARD virtual const SR_MATH_NS::Matrix4x4& GetMatrix() const;

        virtual bool OnResourceReloaded(SR_UTILS_NS::StringAtom resourceId);
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

    protected:
        void OnDestroy() override;
        void OnMatrixDirty() override;
        void OnMaskDirty() override;
        void OnLayerChanged() override;
        void OnPriorityChanged() override;
        void OnEnable() override;
        void OnDisable() override;

        void SetErrorsClean() { m_hasErrors = false; }

        virtual void FreeVMemory();
        virtual bool Calculate();

    private:
        Details::MeshInternalData& GetInternalData() const;

    protected:
        Pipeline* m_pipeline = nullptr;
        int32_t m_virtualUBO = SR_ID_INVALID;
        int32_t m_virtualDescriptor = SR_ID_INVALID;

    protected:
        /// @virtualProperty(verticesCount) @getter(GetVerticesCount) @readOnly @dontSave @debugOnly
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(indicesCount) @getter(GetIndicesCount) @readOnly @dontSave @debugOnly
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(isRegistered) @getter(IsMeshRegistered) @readOnly @dontSave @debugOnly
        SR_VIRTUAL_PROPERTY
        /// @property @setter(SetMaterial) @getter(GetMaterial) @inspector(MaterialPropertyDrawer)
        MaterialPtr m_material;

        bool m_isCalculated          : 1 = false;
        bool m_hasErrors             : 1 = false;
        bool m_dirtyMaterial         : 1 = false;

    private:
        mutable bool m_isAABBDirty   : 1 = true;
        mutable bool m_isMaskDirty   : 1 = true;
        bool m_isDestroyingState     : 1 = false;
        bool m_isWaitReRegister      : 2 = false;

        mutable SR_HTYPES_NS::RawPointerHolder<Details::MeshInternalData> m_pInternalData;
        mutable SR_UTILS_NS::UI::MaskInfo m_maskInfo;

    };

    constexpr static size_t SIZE_OF_MESH_CLASS = sizeof(Mesh);
}

#endif //SR_ENGINE_GRAPHICS_MESH_H
