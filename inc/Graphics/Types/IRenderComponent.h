//
// Created by Monika on 22.05.2023.
//

#ifndef SR_ENGINE_I_RENDER_COMPONENT_H
#define SR_ENGINE_I_RENDER_COMPONENT_H

#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Render/RenderQueue.h>

#include <Utils/ECS/Component.h>
#include <Utils/Math/AABB.h>
#include <Utils/UI/MaskInfo.h>

namespace SR_GRAPH_NS {
    class RenderScene;
}

namespace SR_GTYPES_NS {
    class Camera;

    struct IRenderComponentInternalData {
        SR_MATH_NS::AABB aabb;
        SR_UTILS_NS::VertexLayoutDescription vertexLayoutDescription;
        uint32_t materialRegisterId = SR_ID_INVALID;
        MeshRenderQueues renderQueues;
    };

    /// @hidden @abstract @category(Render)
    class IRenderComponent : public SR_UTILS_NS::Component {
        SR_CLASS()
        using Super = SR_UTILS_NS::Component;
    public:
        using CameraPtr = SR_HTYPES_NS::SharedPtr<Camera>;

    public:
        ~IRenderComponent() override;

        void OnEnable() override;
        void OnDisable() override;
        void OnDestroy() override;
        void OnLayerChanged() override;
        void OnPriorityChanged() override;
        void OnMatrixDirty() override;

        SR_NODISCARD CameraPtr GetCamera() const;
        SR_NODISCARD RenderScene* TryGetRenderScene() const;
        SR_NODISCARD RenderScene* GetRenderScene() const;
        SR_NODISCARD Pipeline* GetPipeline() const noexcept;
        SR_NODISCARD Pipeline* TryGetPipeline() const noexcept;

        SR_NODISCARD virtual std::optional<int32_t> GetIBO() const;
        SR_NODISCARD virtual std::optional<int32_t> GetVBO() const;

        /// registration API
        void ReRegisterRenderObject();
        void UnRegisterRenderObject();
        void OnReRegistered();
        void SetRegistrationInfo(const std::optional<RenderObjectRegistrationInfoInternal>& info);
        SR_NODISCARD RenderObjectRegistrationInfo CreateRegistrationInfo() const;
        SR_NODISCARD bool IsWaitReRegister() const noexcept { return m_isWaitReRegister; }
        SR_NODISCARD bool IsRenderObjectRegistered() const noexcept;
        SR_NODISCARD const RenderObjectRegistrationInfoInternal& GetRegistrationInfo() const noexcept;
        SR_NODISCARD MeshRenderQueues& GetRenderQueues() noexcept;
        SR_NODISCARD RenderQueueInfo* FindRenderQueueInfo(const RenderQueue* pQueue) noexcept;

        /// render API
        void MarkUniformsDirty();
        void MarkMaterialDirty();
        void SetMaterial(const SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::BaseMaterial>& pMaterial);
        void SetMaterial(const SR_UTILS_NS::Path& path);
        virtual void FreeVideoMemory() { }
        virtual void SetVertexLayoutDescription(const SR_UTILS_NS::VertexLayoutDescription& description);
        virtual void UseMaterial(SR_GTYPES_NS::Shader& shader);
        virtual void UseSamplers(SR_GTYPES_NS::Shader& shader);
        virtual void UseModelMatrix(SR_GTYPES_NS::Shader& shader) { }
        SR_NODISCARD const SR_UTILS_NS::VertexLayoutDescription& GetVertexLayoutDescription() const noexcept;
        SR_NODISCARD virtual const SR_UTILS_NS::VertexLayoutDescription& GetShaderVertexLayoutDescription() const noexcept;
        SR_NODISCARD virtual const SR_UTILS_NS::UI::MaskInfo& GetMaskInfo() const;
        SR_NODISCARD const SR_MATH_NS::AABB& GetAABB() const;
        SR_NODISCARD const SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::BaseMaterial>& GetMaterial() const noexcept { return m_material; }
        SR_NODISCARD SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::BaseMaterial>& GetMaterial() noexcept { return m_material; }
        SR_NODISCARD virtual FrustumCullingType GetFrustumCullingType() const noexcept { return FrustumCullingType::None; }
        SR_NODISCARD virtual int32_t GetVirtualUBO() const { return SR_ID_INVALID; }

        virtual void Draw() { }
        virtual bool Bind() { return true; }
        virtual void UseSSBO() { }

    private:
        IRenderComponentInternalData& GetInternalData() const;

    protected:
        bool m_hasErrors     = false;
        bool m_dirtyMaterial = false;

    private:
        bool m_isWaitReRegister    : 2 = false;
        bool m_isDestroyingState   : 2 = false;
        mutable bool m_isAABBDirty : 4 = true;

        /// @property @setter(SetMaterial) @getter(GetMaterial) @inspector(MaterialPropertyDrawer)
        SR_HTYPES_NS::SharedPtr<BaseMaterial> m_material;

        mutable SR_HTYPES_NS::RawPointerHolder<IRenderComponentInternalData> m_internalData;
        mutable RenderScene* m_renderScene = nullptr;
        std::optional<RenderObjectRegistrationInfoInternal> m_registrationInfo;

    };

    /// @hidden @abstract @category(Render)
    class UIRenderComponent : public IRenderComponent {
        SR_CLASS()
        using Super = IRenderComponent;
    public:
        ~UIRenderComponent() override;

    public:
        SR_NODISCARD bool IsActive() const noexcept override;
        SR_NODISCARD int32_t GetVirtualUBO() const override { return m_virtualUBO; }
        SR_NODISCARD const SR_UTILS_NS::UI::MaskInfo& GetMaskInfo() const override;
        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }

        void OnMaskDirty() override;
        void FreeVideoMemory() override;

    protected:
        bool m_isCalculated = false;
        int32_t m_virtualUBO = SR_ID_INVALID;
        int32_t m_virtualDescriptor = SR_ID_INVALID;

    private:
        mutable bool m_isMaskDirty = true;
        mutable SR_UTILS_NS::UI::MaskInfo m_maskInfo;

    };
}

#endif //SR_ENGINE_I_RENDER_COMPONENT_H
