//
// Created by Nikita on 17.11.2020.
//

#ifndef SR_ENGINE_GRAPHICS_MESH_H
#define SR_ENGINE_GRAPHICS_MESH_H

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Common/Enumerations.h>
#include <Utils/Types/SafePointer.h>
#include <Utils/Types/Function.h>

#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Pipeline/IShaderProgram.h>
#include <Graphics/Memory/IGraphicsResource.h>
#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Material/MaterialProperty.h>
#include <Graphics/Memory/DescriptorManager.h>
#include <Graphics/Material/MeshMaterialProperty.h>
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
}

namespace SR_GRAPH_NS {
    class BaseMaterial;
}

namespace SR_GTYPES_NS {
    class Shader;

    class Mesh : public SR_UTILS_NS::NonCopyable, public Memory::IGraphicsResource {
    public:
        using RenderScenePtr = SR_HTYPES_NS::SafePtr<RenderScene>;
        using ShaderPtr = Shader*;
        using MaterialPtr = BaseMaterial*;
        using Ptr = Mesh*;

        struct RenderQueueInfo {
            RenderQueueInfo() = default;
            RenderQueueInfo(RenderQueue* pRenderQueue, const ShaderUseInfo& shaderUseInfo)
                : pRenderQueue(pRenderQueue)
                , shaderUseInfo(shaderUseInfo)
            { }

            RenderQueue* pRenderQueue;
            ShaderUseInfo shaderUseInfo;
            bool operator==(const RenderQueueInfo& other) const {
                return pRenderQueue == other.pRenderQueue;
            }
        };

        struct RenderQueuePredicate {
            using Element = RenderQueueInfo;
            SR_NODISCARD bool operator()(const Element& left, const Element& right) const noexcept {
                return left.pRenderQueue < right.pRenderQueue;
            }
        };

        using RenderQueues = SR_HTYPES_NS::SortedVector<RenderQueueInfo, RenderQueuePredicate>;

    public:
        ~Mesh() override;

    protected:
        explicit Mesh(MeshType type);

    public:
        static std::vector<Mesh::Ptr> Load(const SR_UTILS_NS::Path& path, MeshType type);
        static Mesh::Ptr TryLoad(SR_HTYPES_NS::RawMesh* pRawMesh, MeshType type, uint32_t id);
        static Mesh::Ptr TryLoad(const SR_UTILS_NS::Path& path, MeshType type, uint32_t id);
        static Mesh::Ptr Load(const SR_UTILS_NS::Path& path, MeshType type, uint32_t id);
        static Mesh::Ptr Load(const SR_UTILS_NS::Path& path, MeshType type, SR_UTILS_NS::StringAtom name);

    public:
        SR_NODISCARD virtual int32_t GetIBO() { return SR_ID_INVALID; }
        SR_NODISCARD virtual int32_t GetVBO() { return SR_ID_INVALID; }

        SR_NODISCARD virtual bool IsCalculatable() const;
        SR_NODISCARD virtual bool IsUniqueMesh() const { return false; }

        SR_NODISCARD virtual SR_FORCE_INLINE bool IsMeshActive() const noexcept { return !m_hasErrors; }
        SR_NODISCARD virtual SR_FORCE_INLINE bool IsFlatMesh() const noexcept { return false; }
        SR_NODISCARD virtual std::string GetGeometryName() const { return std::string(); }
        SR_NODISCARD virtual std::string GetMeshIdentifier() const;
        SR_NODISCARD virtual int64_t GetSortingPriority() const { return 0; }
        SR_NODISCARD virtual bool HasSortingPriority() const { return false; }
        SR_NODISCARD virtual SR_UTILS_NS::StringAtom GetMeshLayer() const { return SR_UTILS_NS::StringAtom(); }
        SR_NODISCARD virtual bool IsSupportVBO() const = 0;
        SR_NODISCARD virtual uint32_t GetIndicesCount() const = 0;
        SR_NODISCARD virtual FrustumCullingType GetFrustumCullingType() const { return FrustumCullingType::None; }

        SR_NODISCARD ShaderPtr GetShader() const;
        SR_NODISCARD MeshMaterialProperty& GetMaterialProperty() noexcept { return m_materialProperty; }
        SR_NODISCARD MaterialPtr GetMaterial() const { return m_materialProperty.GetMaterial(); }
        SR_NODISCARD int32_t GetVirtualUBO() const { return m_virtualUBO; }
        SR_NODISCARD MeshType GetMeshType() const noexcept { return m_meshType; }
        SR_NODISCARD bool IsWaitReRegister() const noexcept { return m_isWaitReRegister; }
        SR_NODISCARD bool IsMeshRegistered() const noexcept { return m_registrationInfo.has_value(); }
        SR_NODISCARD bool IsUniformsDirty() const noexcept { return m_isUniformsDirty; }
        SR_NODISCARD const MeshRegistrationInfo& GetMeshRegistrationInfo() const noexcept { return m_registrationInfo.value(); }
        SR_NODISCARD RenderQueues& GetRenderQueues() noexcept { return m_renderQueues; }

        void SetMeshRegistrationInfo(const std::optional<MeshRegistrationInfo>& info) { m_registrationInfo = info; }

        virtual void SetMatrix(const SR_MATH_NS::Matrix4x4& matrix);

        SR_NODISCARD virtual const SR_MATH_NS::Matrix4x4& GetMatrix() const {
            static SR_MATH_NS::Matrix4x4 identity = SR_MATH_NS::Matrix4x4::Identity();
            return identity;
        }

        virtual bool OnResourceReloaded(SR_UTILS_NS::IResource* pResource);
        virtual void SetGeometryName(const std::string& name) { }
        virtual bool BindMesh();

        virtual void Draw();

        virtual void UseMaterial();
        virtual void UseModelMatrix() { }
        virtual void UseSamplers();
        virtual void UseSSBO() { }

        void OnReRegistered();
        void MarkUniformsDirty(bool force = false);
        void MarkMaterialDirty();
        bool DestroyMesh();
        void ReRegisterMesh();
        void UnRegisterMesh();

        void SetMaterial(BaseMaterial* pMaterial);
        void SetMaterial(const SR_UTILS_NS::Path& path);

        void SetErrorsClean() { m_hasErrors = false; }
        void SetUniformsClean() { m_isUniformsDirty = false; }

    protected:
        void FreeVideoMemory() override;

        virtual bool Calculate();

    protected:
        RenderQueues m_renderQueues;

        Memory::UBOManager& m_uboManager;
        SR_GRAPH_NS::DescriptorManager& m_descriptorManager;

        MeshType m_meshType = MeshType::Unknown;

        MeshMaterialProperty m_materialProperty;

        bool m_isWaitReRegister = false;
        bool m_hasErrors = false;
        bool m_dirtyMaterial = false;
        bool m_isUniformsDirty = false;

        int32_t m_virtualUBO = SR_ID_INVALID;
        int32_t m_virtualDescriptor = SR_ID_INVALID;

    private:
        std::optional<MeshRegistrationInfo> m_registrationInfo;

    };
}

#endif //SR_ENGINE_GRAPHICS_MESH_H
