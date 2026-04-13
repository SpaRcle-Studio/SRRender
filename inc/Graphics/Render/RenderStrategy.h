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
    class Mesh;
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
        using MeshPtr = SR_GTYPES_NS::Mesh*;
        using RenderQueuePtr = SR_HTYPES_NS::SharedPtr<RenderQueue>;
    public:
        explicit RenderStrategy(RenderScene* pRenderScene);
        ~RenderStrategy() override;

    public:
        void Prepare();

        void RegisterMesh(SR_GTYPES_NS::Mesh* pMesh);
        bool UnRegisterMesh(SR_GTYPES_NS::Mesh* pMesh);
        void ReRegisterMesh(const MeshRegistrationInfo& info);

        void OnResourceReloaded(SR_UTILS_NS::StringAtom resourceId) const;

        SR_NODISCARD RenderContext* GetRenderContext() const;
        SR_NODISCARD RenderScene* GetRenderScene() const { return m_renderScene; }
        SR_NODISCARD bool IsNeedCheckMeshActivity() const noexcept { return m_isNeedCheckMeshActivity; }
        SR_NODISCARD bool IsDebugModeEnabled() const noexcept { return m_enableDebugMode; }
        SR_NODISCARD bool IsUniformsDirty() const noexcept { return m_isUniformsDirty; }
        SR_NODISCARD const std::set<SR_UTILS_NS::StringAtom>& GetErrors() const noexcept { return m_errors; }
        SR_NODISCARD const std::set<SR_GTYPES_NS::Mesh*>& GetProblemMeshes() const noexcept { return m_problemMeshes; }

        void ClearErrors();
        void AddError(SR_UTILS_NS::StringAtom error) { m_errors.insert(error); }
        void AddProblemMesh(SR_GTYPES_NS::Mesh* pMesh) { m_problemMeshes.insert(pMesh); }
        void SetDebugMode(bool value);

        void MarkUniformsDirty() { m_isUniformsDirty = true; }

        template<class QueueType = RenderQueue, class ReturnType=QueueType> SR_NODISCARD SR_HTYPES_NS::SharedPtr<ReturnType> BuildQueue(MeshDrawerPass* pDrawer);
        void RemoveQueue(RenderQueue* pQueue);

    private:
        void RegisterMesh(const MeshRegistrationInfo& info);
        bool UnRegisterMesh(const MeshRegistrationInfo& info);

        SR_NODISCARD bool BuildQueueImpl(const RenderQueuePtr& pQueue);

        MeshRegistrationInfo CreateMeshRegistrationInfo(SR_GTYPES_NS::Mesh* pMesh);

    private:
        std::vector<RenderQueuePtr> m_queues;

        RenderScene* m_renderScene = nullptr;

        std::set<SR_UTILS_NS::StringAtom> m_errors;
        std::set<SR_GTYPES_NS::Mesh*> m_problemMeshes;

        bool m_isNeedCheckMeshActivity = true;
        bool m_enableDebugMode = false;
        bool m_isUniformsDirty = true;

        std::vector<MeshRegistrationInfo> m_reRegisterMeshes;
        bool m_prepareState = false;

        SR_HTYPES_NS::ObjectPool<MeshPtr, uint32_t> m_meshPool;

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
