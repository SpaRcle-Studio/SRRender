//
// Created by Monika on 02.06.2024.
//

#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Pass/MeshDrawerPass.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Types/Mesh.h>
#include <Graphics/Material/BaseMaterial.h>

#include <Utils/ECS/LayerManager.h>

namespace SR_GRAPH_NS {
    RenderQueue::RenderQueue(RenderStrategy* pStrategy, MeshDrawerPass* pDrawer)
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Manually)
        , m_uboManager(Memory::UBOManager::Instance())
        , m_meshDrawerPass(pDrawer)
        , m_renderStrategy(pStrategy)
    {
        SRAssert(pStrategy && pDrawer);
        m_renderContext = pStrategy->GetRenderContext();
        m_renderScene = pStrategy->GetRenderScene();
        m_pipeline = m_renderContext->GetPipeline().Get();
        m_meshes.reserve(512);
    }

    RenderQueue::~RenderQueue() {
        SR_TRACY_ZONE;

        m_renderStrategy->RemoveQueue(this);

        for (auto&& [layer, queue] : m_queues) {
            for (auto&& meshInfo : queue) {
                meshInfo.pMesh->GetRenderQueues().Remove(RenderQueueInfo { .pRenderQueue = this, .pShader = nullptr });
                if (meshInfo.pMesh->GetRenderQueues().empty()) {
                    meshInfo.pMesh->SetUniformsClean();
                }
            }
        }
    }

    void RenderQueue::Register(const MeshRegistrationInfo& info) {
        SR_TRACY_ZONE;

        if (!IsSuitable(info)) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        PrepareLayers();

        MeshInfo meshInfo;
        meshInfo.pMesh = info.pMesh;
        meshInfo.pShader = info.pMaterial ? info.pMaterial->GetShader(m_meshDrawerPass->GetShaderMacros()) : nullptr;
        meshInfo.vbo = info.VBO.has_value() ? info.VBO.value() : SR_ID_INVALID;
        meshInfo.priority = info.priority.value_or(0);

        info.pMesh->GetRenderQueues().Add(RenderQueueInfo { .pRenderQueue = this, .pShader = meshInfo.pShader });

        for (auto&& [layer, queue] : m_queues) {
            if (layer == info.layer) {
                queue.Add(meshInfo);
                break;
            }
        }
    }

    void RenderQueue::UnRegister(const MeshRegistrationInfo& info) {
        SR_TRACY_ZONE;

        RenderQueue::Queue* pQueue = nullptr;

        for (auto&& [layer, queue] : m_queues) {
            if (layer == info.layer) {
                pQueue = &queue;
                break;
            }
        }

        if (!pQueue) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        if (info.priority.has_value() && !m_meshDrawerPass->IsPriorityAllowed(info.priority.value())) {
            return;
        }

        MeshInfo meshInfo;
        meshInfo.pMesh = info.pMesh;
        meshInfo.vbo = info.VBO.has_value() ? info.VBO.value() : SR_ID_INVALID;
        meshInfo.priority = info.priority.value_or(0);

        auto&& queues = info.pMesh->GetRenderQueues();
        auto&& pIt = queues.LowerBound(RenderQueueInfo { .pRenderQueue = this, .pShader = nullptr });

        if (pIt == queues.end()) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("RenderQueue::UnRegister() : queue not found!");
            return;
        }

        meshInfo.pShader = pIt->pShader;
        queues.Erase(pIt);

        if (!pQueue->Remove(meshInfo)) {
            SRHalt("RenderQueue::UnRegister() : mesh not found!");
        }

        if (queues.empty()) {
            meshInfo.pMesh->SetUniformsClean();

            for (auto it = m_meshes.begin(); it != m_meshes.end(); ) {
                if (it->pMesh == meshInfo.pMesh) {
                    it = m_meshes.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
    }

    void RenderQueue::Init() {
        SRAssert(!m_isInitialized);
        m_isInitialized = true;
        m_multiFrameMode = SR_UTILS_NS::Features::Instance().Enabled("MultiFrameResources", true);
        m_updateMeshesOnDemand = SR_UTILS_NS::Features::Instance().Enabled("UpdateMeshesOnDemand", false);
    }

    bool RenderQueue::Render() {
        SR_TRACY_ZONE;
        SRAssert(m_isInitialized);

        m_pipeline->SetRenderStageId(m_renderStageId);

        PrepareLayers();

        m_rendered = false;

        m_shaders.Clear();

        for (auto&& [layer, queue] : m_queues) {
            Render(layer, queue);
        }

        return m_rendered;
    }

    void RenderQueue::Update() {
        SR_TRACY_ZONE;

        if (!m_rendered) {
            return;
        }

        UpdateShaders();

        if (m_updateMeshesOnDemand) {
            UpdateMeshes();
        }
        else {
            UpdateAllMeshes();
        }
    }

    void RenderQueue::OnMeshDirty(SR_GTYPES_NS::Mesh* pMesh, SR_GTYPES_NS::Shader* pShader) {
        m_meshes.emplace_back(MeshShaderPair { .pMesh = pMesh, .pShader = pShader });
    }

    void RenderQueue::UpdateShaders() {
        SR_TRACY_ZONE;

        auto pStart = m_shaders.data();
        auto pEnd = pStart + m_shaders.size();

        for (auto* pElement = pStart; pElement < pEnd; ++pElement) {
            if ((*pElement)->BeginSharedUBO()) SR_LIKELY_ATTRIBUTE {
                m_meshDrawerPass->UseSharedUniforms(*pElement);
                (*pElement)->EndSharedUBO();
            }
        }
    }

    void RenderQueue::UpdateMeshes() {
        SR_TRACY_ZONE;

        m_tempMeshes.clear();

        const uint8_t frameIndex = m_pipeline->GetCurrentFrameIndex();
        const uint8_t maxFrames = m_pipeline->GetSwapchainImagesCount();

        auto pStart = m_meshes.data();
        auto pEnd = pStart + m_meshes.size();

        for (auto* pElement = pStart; pElement < pEnd; ++pElement) {
            const auto pMesh = pElement->pMesh;
            const auto pShader = pElement->pShader;

            pElement->updatedFrames[frameIndex] = true;

            bool isNeedReUpdate = false;
            if (m_multiFrameMode) {
                for (uint8_t i = 0; i < maxFrames; ++i) {
                    if (!pElement->updatedFrames[i]) SR_UNLIKELY_ATTRIBUTE {
                        isNeedReUpdate = true;
                        break;
                    }
                }
            }

            if (!isNeedReUpdate) {
                pMesh->SetUniformsClean();
            }
            else {
                m_tempMeshes.emplace_back(*pElement);
            }

            auto&& virtualUbo = pMesh->GetVirtualUBO();
            if (virtualUbo == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            m_pipeline->SetCurrentShader(pShader);

            /// Если меш не был отрисован, то бинд не пройдет
            if (m_uboManager.BindNoDublicateUBO(virtualUbo) == Memory::UBOManager::BindResult::Success) SR_UNLIKELY_ATTRIBUTE {
                m_meshDrawerPass->UseUniforms(pShader, pMesh);
                SR_MAYBE_UNUSED_VAR pShader->Flush();
            }
        }

        m_meshes.clear();

        if (m_multiFrameMode) {
            m_meshes = m_tempMeshes;
        }
    }

    bool RenderQueue::IsSuitable(const MeshRegistrationInfo &info) const {
        SR_TRACY_ZONE;

        if (!m_meshDrawerPass->IsLayerAllowed(info.layer)) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }

        if (info.priority.has_value() && !m_meshDrawerPass->IsPriorityAllowed(info.priority.value())) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }

        return true;
    }

    void RenderQueue::Render(const SR_UTILS_NS::StringAtom& layer, RenderQueue::Queue& queue) {
        SR_TRACY_ZONE_S(layer.c_str());

        SR_GTYPES_NS::Shader* pCurrentShader = nullptr;
        VBO currentVBO = 0;

        MeshInfo* pStart = queue.data();
        const MeshInfo* pEnd = pStart + queue.size();
        bool shaderOk = false;

        for (MeshInfo* pElement = pStart; pElement < pEnd; ) {
            const MeshInfo info = *pElement;

            if (info.pMesh->IsWaitReRegister()) SR_UNLIKELY_ATTRIBUTE {
                pElement->state = QUEUE_STATE_WAIT_REGISTER;
                ++pElement;
                continue;
            }

            if (!info.pShader) SR_UNLIKELY_ATTRIBUTE {
                pElement->state = QUEUE_STATE_MISSING_SHADER;
                ++pElement;
                continue;
            }

            const bool invalidVBO = info.vbo == SR_INVALID_VBO && info.pMesh->IsSupportVBO();
            if (invalidVBO) SR_UNLIKELY_ATTRIBUTE {
                pElement->state = QUEUE_STATE_VBO_ERROR;
                ++pElement;
                continue;
            }

            if (info.pShader != pCurrentShader) SR_UNLIKELY_ATTRIBUTE {
                pCurrentShader = info.pShader;
                shaderOk = UseShader(pCurrentShader);
                currentVBO = SR_ID_INVALID;
                if (!shaderOk) SR_UNLIKELY_ATTRIBUTE {
                    pElement->state = QUEUE_STATE_SHADER_ERROR;
                    pElement = FindNextShader(queue, pElement);
                    continue;
                }

                const auto pIt = m_shaders.LowerBound(pCurrentShader);
                if (pIt == m_shaders.end() || *pIt != pCurrentShader) {
                    m_shaders.Insert(pIt, pCurrentShader);
                }
            }

            if (info.vbo != currentVBO) SR_UNLIKELY_ATTRIBUTE {
                if (!info.pMesh->BindMesh()) SR_UNLIKELY_ATTRIBUTE {
                    pElement->state = QUEUE_STATE_VBO_ERROR;
                    pElement = FindNextVBO(queue, pElement);
                    continue;
                }
                currentVBO = info.vbo;
            }

            if (m_customMeshDraw) SR_UNLIKELY_ATTRIBUTE {
                CustomDrawMesh(info);
            }
            else {
                info.pMesh->Draw();
            }

            pElement->state = QUEUE_STATE_OK;
            ++pElement;
            m_rendered = true;
        }

        if (pCurrentShader && shaderOk) SR_LIKELY_ATTRIBUTE {
            pCurrentShader->UnUse();
        }
    }

    RenderQueue::MeshInfo* RenderQueue::FindNextShader(Queue& queue, MeshInfo* pElement) {
        SR_TRACY_ZONE;

        auto pEnd = queue.data() + queue.size();
        auto pShader = pElement->pShader;

        ++pElement;

        while (pElement != pEnd) SR_UNLIKELY_ATTRIBUTE {
            if (pElement->pShader != pShader) SR_UNLIKELY_ATTRIBUTE {
                return pElement;
            }
            ++pElement;
        }

        return pEnd;
    }

    RenderQueue::MeshInfo* RenderQueue::FindNextVBO(Queue& queue, MeshInfo* pElement) {
        SR_TRACY_ZONE;

        auto pEnd = queue.data() + queue.size();
        auto vbo = pElement->vbo;

        while (pElement != pEnd) {
            if (pElement->vbo != vbo) {
                return pElement;
            }
            ++pElement;
        }

        return pEnd;
    }

    bool RenderQueue::UseShader(SR_GTYPES_NS::Shader* pShader) {
        SR_TRACY_ZONE;

        if (pShader->Use() == ShaderBindResult::Failed) {
            return false;
        }

        m_renderContext->SetCurrentShader(pShader);

        m_meshDrawerPass->UseConstants(pShader);

        if (m_pipeline->IsShaderChanged()) {
            m_meshDrawerPass->UseSamplers(pShader);
        }

        if (!pShader->IsSamplersValid()) {
            SR_TRACY_ZONE_S("Shader error");
            SR_TRACY_ZONE_COLOR(0xFF0000);

            std::string message = "Shader samplers is not valid!\n\tPath: " + pShader->GetResourcePath().ToStringRef();
            for (auto&& [name, sampler] : pShader->GetSamplers()) {
                if (m_pipeline->IsSamplerValid(sampler.samplerId)) {
                    continue;
                }

                message += "\n\tSampler is not set: " + name.ToStringRef();
            }
            m_renderStrategy->AddError(message);
            pShader->UnUse();
            return false;
        }

        return true;
    }

    void RenderQueue::PrepareLayers() {
        SR_TRACY_ZONE;

        auto&& layerManager = SR_UTILS_NS::LayerManager::Instance();

        if (layerManager.GetHashState() == m_layersStateHash) {
            return;
        }

        SR_MAYBE_UNUSED auto&& guard = SR_UTILS_NS::LayerManager::ScopeLockSingleton();

        m_layersStateHash = layerManager.GetHashState();

        auto stash = std::move(m_queues);

        for (auto&& layer : layerManager.GetLayers()) {
            if (!m_meshDrawerPass->IsLayerAllowed(layer)) {
                continue;
            }
            m_queues.emplace_back(layer, Queue());
        }

        for (auto&& [layer, queue] : stash) {
            for (auto&& [newLayer, newQueue] : m_queues) {
                if (layer == newLayer) {
                    newQueue = std::move(queue);
                    break;
                }
            }
        }
    }

    void RenderQueue::UpdateAllMeshes() {
        SR_TRACY_ZONE;

        m_meshes.clear();

        for (auto&& [layer, queue] : m_queues) {
            for (auto&& meshInfo : queue) {
                auto&& virtualUbo = meshInfo.pMesh->GetVirtualUBO();
                if (virtualUbo == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                    continue;
                }

                m_pipeline->SetCurrentShader(meshInfo.pShader);

                /// Если меш не был отрисован, то бинд не пройдет
                if (m_uboManager.BindNoDublicateUBO(virtualUbo) == Memory::UBOManager::BindResult::Success) SR_UNLIKELY_ATTRIBUTE {
                    m_meshDrawerPass->UseUniforms(meshInfo.pShader, meshInfo.pMesh);
                    SR_MAYBE_UNUSED_VAR meshInfo.pShader->Flush();
                }
            }
        }
    }
}
