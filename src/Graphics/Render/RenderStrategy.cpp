//
// Created by Monika on 20.01.2024.
//

#include <Graphics/Render/RenderStrategy.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Pass/MeshDrawerPass.h>
#include <Graphics/Types/IRenderComponent.h>

#include <Utils/ECS/LayerManager.h>

namespace SR_GRAPH_NS {
    RenderStrategy::RenderStrategy(RenderScene* pRenderScene)
        : Super()
        , m_renderScene(pRenderScene)
    { }

    RenderStrategy::~RenderStrategy() {
        SRAssert(m_objectPool.IsEmpty());
        SRAssert(m_queues.empty());
        SRAssert(m_reRegisterQueue.empty());
    }

    RenderContext* RenderStrategy::GetRenderContext() const {
        return m_renderScene->GetContext();
    }

    void RenderStrategy::Prepare() {
        SR_TRACY_ZONE;

        if (m_reRegisterQueue.empty()) {
            return;
        }

        GetRenderContext()->GetPipeline()->SetDirty(true);

        m_prepareState = true;

        for (auto&& info : m_reRegisterQueue) {
            bool inUpdateQueue = false;
            for (auto&& pQueue : m_queues) {
                SRAssert(pQueue);
                pQueue->UnRegister(info, &inUpdateQueue);
            }

            m_objectPool.RemoveByIndex(info.internal.poolId);

            Register(CreateRegistrationInfo(info.pObject));
            info.pObject->OnReRegistered();
            if (inUpdateQueue) {
                info.pObject->MarkUniformsDirty();
            }
        }
        m_reRegisterQueue.clear();

        m_prepareState = false;
    }

    void RenderStrategy::Register(RenderObjectPtr pObject) {
        SR_TRACY_ZONE;

        if (pObject->IsRenderObjectRegistered()) {
            SRHalt("Double registration!");
            return;
        }

        Register(CreateRegistrationInfo(pObject));
    }

    bool RenderStrategy::UnRegister(RenderObjectPtr pObject) {
        SR_TRACY_ZONE;

        if (IsDebugModeEnabled() && m_problemObjects.count(pObject) == 1) {
            m_problemObjects.erase(pObject);
        }

        if (!pObject->IsRenderObjectRegistered()) {
            SRHalt("Object is not registered!");
            return false;
        }

        UnRegisterImpl(pObject, pObject->GetRegistrationInfo());

        return true;
    }

    void RenderStrategy::RemoveQueue(RenderQueue* pQueue) {
        for (auto pIt = m_queues.begin(); pIt != m_queues.end(); ++pIt) {
            if (pIt->Get() == pQueue) {
                m_queues.erase(pIt);
                return;
            }
        }
        SRHalt("Queue not found!");
    }

    void RenderStrategy::Register(const RenderObjectRegistrationInfo& info) {
        for (auto&& pQueue : m_queues) {
            SRAssert(pQueue);
            pQueue->Register(info);
        }

        info.pObject->SetRegistrationInfo(info.internal);
    }

    bool RenderStrategy::UnRegisterImpl(RenderObjectPtr pObject, const RenderObjectRegistrationInfoInternal& info) {
        SRAssert2(!m_prepareState, "UnRegisterImpl() is not allowed during Prepare()!");

        if (pObject->IsWaitReRegister()) {
            SR_MAYBE_UNUSED bool isFound = false;
            for (auto pIt = m_reRegisterQueue.begin(); pIt != m_reRegisterQueue.end(); ++pIt) {
                if (pIt->pObject == pObject) {
                    m_reRegisterQueue.erase(pIt);
                    isFound = true;
                    pObject->OnReRegistered();
                    break;
                }
            }
            SRAssert2(isFound, "Mesh is not found in re-register list, but it is waiting for re-register!");
        }

        RenderObjectRegistrationInfo infoEx;
        infoEx.pObject = pObject;
        infoEx.internal = info;

        for (auto&& pQueue : m_queues) {
            SRAssert(pQueue);
            pQueue->UnRegister(infoEx, nullptr);
        }

        m_objectPool.RemoveByIndex(info.poolId);
        pObject->SetRegistrationInfo(std::nullopt);

        return true;
    }

    bool RenderStrategy::BuildQueueImpl(const RenderQueuePtr& pQueue) {
        SR_TRACY_ZONE;
        pQueue->Init();
        m_objectPool.ForEach([pQueue](uint32_t id, const RenderObjectPtr& pObject) {
            RenderObjectRegistrationInfo info;
            info.pObject = pObject;
            info.internal = pObject->GetRegistrationInfo();
            pQueue->Register(info);
        });
        return true;
    }

    void RenderStrategy::OnResourceReloaded(SR_UTILS_NS::StringAtom resourceId) const {
        SR_TRACY_ZONE;
    }

    void RenderStrategy::SetDebugMode(bool value) {
        m_enableDebugMode = value;

        if (!m_enableDebugMode) {
            ClearErrors();
        }

        m_renderScene->SetDirty();
    }

    void RenderStrategy::ClearErrors() {
        m_problemObjects.clear();
        m_errors.clear();
    }

    void RenderStrategy::ReRegister(const RenderObjectRegistrationInfo& info) {
        SR_TRACY_ZONE;
        SRAssert2(!m_prepareState, "ReRegisterMesh() is not allowed during Prepare()!");
        m_reRegisterQueue.emplace_back(info);
    }

    RenderObjectRegistrationInfo RenderStrategy::CreateRegistrationInfo(RenderObjectPtr pObject) {
        RenderObjectRegistrationInfo info = pObject->CreateRegistrationInfo();
        info.internal.poolId = m_objectPool.Add(pObject);
        return info;
    }
}