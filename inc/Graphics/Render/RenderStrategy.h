//
// Created by Monika on 17.01.2024.
//

#ifndef SR_ENGINE_RENDER_STRATEGY_H
#define SR_ENGINE_RENDER_STRATEGY_H

#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Pipeline/ShaderUtils.h>
#include <Graphics/Render/RenderPredicates.h>

#include <Utils/ECS/Transform.h>

/**
    - Layer (Only render)
        - Priority (Only render)
            - Shader (Render/Update)
                * OnShaderUse
                - VBO (Render/Update)
                    * OnBindVBO
                    - Mesh
                    - Mesh
                    * OnUnBingVBO
               * OnShaderUnUse
*/

namespace SR_GTYPES_NS {
    class IRenderComponent;
    class Shader;
}

namespace SR_GRAPH_NS {
    class RenderStrategy;
    class RenderScene;
    class RenderQueue;
    class MeshDrawerPass;
    class RenderContext;

    class RenderStrategy : public SR_UTILS_NS::NonCopyable {
        using Super = SR_UTILS_NS::NonCopyable;
        using ShaderPtr = SR_GTYPES_NS::Shader*;
        using RenderObjectPtr = SR_GTYPES_NS::IRenderComponent*;
        using RenderQueuePtr = SR_HTYPES_NS::SharedPtr<RenderQueue>;
    public:
        explicit RenderStrategy(RenderScene* pRenderScene);
        ~RenderStrategy() override;

    public:
        void Prepare();

        void Register(RenderObjectPtr pObject);
        bool UnRegister(RenderObjectPtr pObject);
        void ReRegister(const RenderObjectRegistrationInfo& info);

        void OnResourceReloaded(SR_UTILS_NS::StringAtom resourceId) const;

        SR_NODISCARD RenderContext* GetRenderContext() const;
        SR_NODISCARD RenderScene* GetRenderScene() const { return m_renderScene; }
        SR_NODISCARD bool IsNeedCheckMeshActivity() const noexcept { return m_isNeedCheckMeshActivity; }
        SR_NODISCARD bool IsDebugModeEnabled() const noexcept { return m_enableDebugMode; }
        SR_NODISCARD bool IsUniformsDirty() const noexcept { return m_isUniformsDirty; }
        SR_NODISCARD const std::set<SR_UTILS_NS::StringAtom>& GetErrors() const noexcept { return m_errors; }
        SR_NODISCARD const std::set<RenderObjectPtr>& GetProblemObjects() const noexcept { return m_problemObjects; }

        void ClearErrors();
        void AddError(SR_UTILS_NS::StringAtom error) { m_errors.insert(error); }
        void SetDebugMode(bool value);

        void MarkUniformsDirty() { m_isUniformsDirty = true; }

        template<class QueueType = RenderQueue, class ReturnType=QueueType> SR_NODISCARD SR_HTYPES_NS::SharedPtr<ReturnType> BuildQueue(MeshDrawerPass* pDrawer);
        void RemoveQueue(RenderQueue* pQueue);

    private:
        void Register(const RenderObjectRegistrationInfo& info);
        bool UnRegisterImpl(RenderObjectPtr pObject, const RenderObjectRegistrationInfoInternal& info);

        SR_NODISCARD bool BuildQueueImpl(const RenderQueuePtr& pQueue);
        RenderObjectRegistrationInfo CreateRegistrationInfo(RenderObjectPtr pObject);

    private:
        std::vector<RenderQueuePtr> m_queues;

        RenderScene* m_renderScene = nullptr;

        std::set<SR_UTILS_NS::StringAtom> m_errors;
        std::set<RenderObjectPtr> m_problemObjects;

        bool m_isNeedCheckMeshActivity = true;
        bool m_enableDebugMode = false;
        bool m_isUniformsDirty = true;

        std::vector<RenderObjectRegistrationInfo> m_reRegisterQueue;
        bool m_prepareState = false;

        SR_HTYPES_NS::ObjectPool<RenderObjectPtr, uint32_t> m_objectPool;

    };

    template<class QueueType, class ReturnType> SR_HTYPES_NS::SharedPtr<ReturnType> RenderStrategy::BuildQueue(MeshDrawerPass* pDrawer) {
        SR_STATIC_ASSERT2((std::is_base_of_v<RenderQueue, QueueType>), "QueueType must be derived from RenderQueue");

        auto&& pQueue = RenderQueuePtr::MakeShared<QueueType, ReturnType>(this, pDrawer);

        if (!BuildQueueImpl(pQueue)) {
            SRHalt("Failed to build queue");
            pQueue.AutoFree();
            return nullptr;
        }

        m_queues.emplace_back(pQueue);

        return pQueue;
    }
}

#endif //SR_ENGINE_RENDER_STRATEGY_H
