//
// Created by Monika on 20.01.2024.
//

#include <Graphics/Render/RenderStrategy.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Pass/MeshDrawerPass.h>
#include <Graphics/Types/Mesh.h>

#include <Utils/ECS/LayerManager.h>

namespace SR_GRAPH_NS {
    RenderStrategy::RenderStrategy(RenderScene* pRenderScene)
        : Super()
        , m_renderScene(pRenderScene)
    { }

    RenderStrategy::~RenderStrategy() {
        SRAssert(m_meshPool.IsEmpty());
        SRAssert(m_queues.empty());
        SRAssert(m_reRegisterMeshes.empty());
    }

    RenderContext* RenderStrategy::GetRenderContext() const {
        return m_renderScene->GetContext();
    }

    void RenderStrategy::Prepare() {
        SR_TRACY_ZONE;

        if (m_reRegisterMeshes.empty()) {
            return;
        }

        GetRenderContext()->GetPipeline()->SetDirty(true);

        m_prepareState = true;

        for (auto&& info : m_reRegisterMeshes) {
            for (auto&& pQueue : m_queues) {
                SRAssert(pQueue);
                pQueue->UnRegister(info);
            }

            m_meshPool.RemoveByIndex(info.poolId);

            RegisterMesh(CreateMeshRegistrationInfo(info.pMesh));
            info.pMesh->OnReRegistered();
        }
        m_reRegisterMeshes.clear();

        m_prepareState = false;
    }

    void RenderStrategy::RegisterMesh(SR_GTYPES_NS::Mesh* pMesh) {
        SR_TRACY_ZONE;

        if (pMesh->IsMeshRegistered()) {
            SRHalt("Double mesh registration!");
            return;
        }

        RegisterMesh(CreateMeshRegistrationInfo(pMesh));
    }

    bool RenderStrategy::UnRegisterMesh(SR_GTYPES_NS::Mesh* pMesh) {
        SR_TRACY_ZONE;

        if (IsDebugModeEnabled() && m_problemMeshes.count(pMesh) == 1) {
            m_problemMeshes.erase(pMesh);
        }

        if (!pMesh->IsMeshRegistered()) {
            SRHalt("Mesh is not registered!");
            return false;
        }

        UnRegisterMesh(pMesh->GetMeshRegistrationInfo());

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

    void RenderStrategy::RegisterMesh(const MeshRegistrationInfo& info) {
        for (auto&& pQueue : m_queues) {
            SRAssert(pQueue);
            pQueue->Register(info);
        }

        info.pMesh->SetMeshRegistrationInfo(info);
    }

    bool RenderStrategy::UnRegisterMesh(const MeshRegistrationInfo& info) {
        SRAssert2(!m_prepareState, "UnRegisterMesh() is not allowed during Prepare()!");

        if (info.pMesh->IsWaitReRegister()) {
            SR_MAYBE_UNUSED bool isFound = false;
            for (auto pIt = m_reRegisterMeshes.begin(); pIt != m_reRegisterMeshes.end(); ++pIt) {
                if (pIt->pMesh == info.pMesh) {
                    m_reRegisterMeshes.erase(pIt);
                    isFound = true;
                    info.pMesh->OnReRegistered();
                    break;
                }
            }
            SRAssert2(isFound, "Mesh is not found in re-register list, but it is waiting for re-register!");
        }

        for (auto&& pQueue : m_queues) {
            SRAssert(pQueue);
            pQueue->UnRegister(info);
        }

        m_meshPool.RemoveByIndex(info.poolId);

        info.pMesh->SetMeshRegistrationInfo(std::nullopt);

        return true;
    }

    bool RenderStrategy::BuildQueueImpl(const RenderQueuePtr& pQueue) {
        pQueue->Init();
        m_meshPool.ForEach([pQueue](uint32_t id, const MeshPtr& pMesh) {
            pQueue->Register(pMesh->GetMeshRegistrationInfo());
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
        m_problemMeshes.clear();
        m_errors.clear();
    }

    void RenderStrategy::ReRegisterMesh(const MeshRegistrationInfo& info) {
        SR_TRACY_ZONE;
        SRAssert2(!m_prepareState, "ReRegisterMesh() is not allowed during Prepare()!");
        m_reRegisterMeshes.emplace_back(info);
    }

    MeshRegistrationInfo RenderStrategy::CreateMeshRegistrationInfo(SR_GTYPES_NS::Mesh* pMesh) {
        MeshRegistrationInfo info = { };

        info.pMesh = pMesh;
        info.pMaterial = pMesh->GetMaterial().Get();
        info.layer = pMesh->GetMeshLayer();
        info.poolId = m_meshPool.Add(pMesh);

        if (pMesh->IsSupportVBO()) {
            info.VBO = pMesh->GetVBO();
        }

        if (pMesh->HasSortingPriority()) {
            info.priority = pMesh->GetSortingPriority();
        }

        return info;
    }
}