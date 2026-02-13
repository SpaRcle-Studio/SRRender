//
// Created by Monika on 02.06.2024.
//

#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Pass/MeshDrawerPass.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Types/Mesh.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/Utils/Frustum.h>
#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Window/Window.h>

#include <Utils/ECS/LayerManager.h>
#include <Utils/Common/Features.h>

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
        m_queues.reserve(32);
    }

    RenderQueue::~RenderQueue() {
        SR_TRACY_ZONE;

        m_renderStrategy->RemoveQueue(this);

        for (auto&& [layer, queue] : m_queues) {
            for (auto&& meshInfo : queue) {
                meshInfo.pMesh->GetRenderQueues().Remove(this);
            }
        }
    }

    void RenderQueue::Register(const MeshRegistrationInfo& info) {
        SR_TRACY_ZONE;

        if (!IsSuitable(info)) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        PrepareLayers();

        auto&& pNewQueue = info.pMesh->GetRenderQueues().Add(this);
        if (!pNewQueue) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("RenderQueue::Register() : mesh already registered!");
            return;
        }
        pNewQueue->pShader = info.pMaterial ? info.pMaterial->GetShader(m_meshDrawerPass->GetShaderMacros()) : nullptr;

        MeshInfo meshInfo;
        meshInfo.pMesh = info.pMesh;
        meshInfo.pShader = pNewQueue->pShader;
        meshInfo.pInfo = pNewQueue;
        meshInfo.vbo = info.VBO.has_value() ? info.VBO.value() : SR_ID_INVALID;
        meshInfo.priority = info.priority.value_or(0);

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

        auto&& queues = info.pMesh->GetRenderQueues();

        auto removedInfo = queues.Remove(this);

        MeshInfo meshInfo;
        meshInfo.pMesh = info.pMesh;
        meshInfo.vbo = info.VBO.has_value() ? info.VBO.value() : SR_ID_INVALID;
        meshInfo.priority = info.priority.value_or(0);
        meshInfo.pShader = removedInfo.pShader;

        if (!pQueue->Remove(meshInfo)) {
            SRHalt("RenderQueue::UnRegister() : mesh not found!");
        }

        if (removedInfo.inUpdateQueue) {
            for (auto it = m_meshes.begin(); it != m_meshes.end();) {
                if (it->pMesh == meshInfo.pMesh) {
                    it = m_meshes.erase(it);
                } else {
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

        m_pipeline->SetCurrentCamera(m_meshDrawerPass->GetCamera());

        UpdateShaders();

        if (m_updateMeshesOnDemand) {
            UpdateMeshes();
        }
        else {
            UpdateAllMeshes();
        }
    }

    void RenderQueue::OnMeshDirty(SR_GTYPES_NS::Mesh* pMesh, RenderQueueInfo* pInfo) {
        m_meshes.emplace_back(MeshShaderPair { .pMesh = pMesh, .pInfo = pInfo });
    }

    void RenderQueue::UpdateShaders() {
        SR_TRACY_ZONE;

        auto pStart = m_shaders.data();
        auto pEnd = pStart + m_shaders.size();

        for (auto* pElement = pStart; pElement < pEnd; ++pElement) {
            if ((*pElement)->BeginSharedUBO()) SR_LIKELY_ATTRIBUTE {
                m_meshDrawerPass->UseSharedUniforms(**pElement);
                (*pElement)->EndSharedUBO();
            }
        }
    }

    void RenderQueue::UpdateMeshes() {
        SR_TRACY_ZONE;

        m_tempMeshes.clear();

        const uint8_t frameIndex = m_pipeline->GetCurrentImageIndex();
        const uint8_t maxFrames = m_pipeline->GetSwapchainImagesCount();

        auto pStart = m_meshes.data();
        auto pEnd = pStart + m_meshes.size();

        for (auto* pElement = pStart; pElement < pEnd; ++pElement) {
            if (!pElement->pInfo->isVisible) SR_UNLIKELY_ATTRIBUTE {
                pElement->pInfo->inUpdateQueue = false;
                continue;
            }

            const auto pMesh = pElement->pMesh;

            bool isNeedReUpdate = false;

            if (m_multiFrameMode) {
                pElement->pInfo->dirtyUniformsFrames[frameIndex] = false;
                for (uint8_t i = 0; i < maxFrames; ++i) {
                    if (pElement->pInfo->dirtyUniformsFrames[i]) {
                        isNeedReUpdate = true;
                        break;
                    }
                }
            }

            if (!isNeedReUpdate) {
                pElement->pInfo->inUpdateQueue = false;
            }
            else {
                m_tempMeshes.emplace_back(*pElement);
            }

            auto&& virtualUbo = pMesh->GetVirtualUBO();
            if (virtualUbo == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            const auto pShader = pElement->pInfo->pShader;
            m_pipeline->SetCurrentShader(pShader);

            /// Если меш не был отрисован, то бинд не пройдет
            if (m_uboManager.BindNoDublicateUBO(virtualUbo) == Memory::UBOManager::BindResult::Success) SR_UNLIKELY_ATTRIBUTE {
                m_meshDrawerPass->UseUniforms(*pShader, pMesh);
                SR_MAYBE_UNUSED_VAR pShader->Flush();
            }
        }

        m_meshes.clear();

        if (m_multiFrameMode) {
            std::swap(m_tempMeshes, m_meshes);
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

        auto&& pCurrentFramebuffer = m_pipeline->GetCurrentFrameBuffer();
        SR_MATH_NS::IVector2 viewportSize;
        if (pCurrentFramebuffer) {
            viewportSize = pCurrentFramebuffer->GetSize();
        }
        else if (auto&& pWindow = m_pipeline->GetWindow()) {
            viewportSize = pWindow->GetSize().CastToInt();
        }

        MeshInfo* pStart = queue.data();
        const MeshInfo* pEnd = pStart + queue.size();
        bool shaderOk = false;

        for (MeshInfo* pElement = pStart; pElement < pEnd; ) {
            const MeshInfo info = *pElement;

            if (!info.pInfo->isVisible) SR_UNLIKELY_ATTRIBUTE {
                pElement->state = QUEUE_STATE_INVISIBLE;
                ++pElement;
                continue;
            }

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
                    pElement->pMesh->MarkMaterialDirty();
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

            const SR_UTILS_NS::UI::MaskInfo& mask = info.pMesh->GetMaskInfo();
            const bool hasMask = mask.hasMask && mask.scissor;
            if (hasMask) {
                SR_MATH_NS::FVector2 multiplier = viewportSize.CastToFloat() / mask.referenceSize.CastToFloat();
                const auto rect = SR_MATH_NS::FRect(
                    static_cast<float_t>(mask.rect.x) * multiplier.x, static_cast<float_t>(mask.rect.y) * multiplier.y,
                    static_cast<float_t>(mask.rect.w) * multiplier.x, static_cast<float_t>(mask.rect.h) * multiplier.y
                ).CastToInt();
                m_pipeline->PushScissor(rect);
            }

            if (m_customMeshDraw) SR_UNLIKELY_ATTRIBUTE {
                CustomDrawMesh(info);
            }
            else {
                info.pMesh->Draw();
            }

            if (hasMask) {
                m_pipeline->PopScissor();
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

        m_meshDrawerPass->UseConstants(*pShader);

        if (m_pipeline->IsShaderChanged()) {
            m_meshDrawerPass->UseSamplers(*pShader);
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
            m_queues.back().second.Reserve(512);
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
                    m_meshDrawerPass->UseUniforms(*meshInfo.pShader, meshInfo.pMesh);
                    SR_MAYBE_UNUSED_VAR meshInfo.pShader->Flush();
                }
            }
        }
    }

    bool RenderQueue::UpdateFrustumCulling(const Frustum& frustum) {
        SR_TRACY_ZONE;

        if (!m_isFrustumAllowed) {
            return false;
        }

        bool changed = false;

        for (auto&& queue : m_queues | std::views::values) {
            for (auto&& meshInfo : queue) {
                bool isVisible = true;

                if (!m_checkMeshFrustumSupport || meshInfo.pMesh->IsFrustumCullingSupported()) {
                    const FrustumCullingType type = meshInfo.pMesh->GetFrustumCullingType();

                    if (type == FrustumCullingType::None) {
                        /// nothing
                    }
                    else { /// TODO: support other types of frustum culling
                        isVisible = frustum.IsAABBVisible(meshInfo.pMesh->GetAABB());
                    }
                }

                if (meshInfo.pInfo->isVisible != isVisible) {
                    meshInfo.pInfo->isVisible = isVisible;
                    if (isVisible) {
                        meshInfo.pInfo->dirtyUniformsFrames.set();
                        if (!meshInfo.pInfo->inUpdateQueue) {
                            meshInfo.pInfo->pRenderQueue->OnMeshDirty(meshInfo.pMesh, meshInfo.pInfo);
                        }
                    }
                    changed = true;
                }
            }
        }
        return changed;
    }

    bool RenderQueue::MeshInfo::operator==(const RenderQueue::MeshInfo &other) const noexcept {
        return
            pShader == other.pShader &&
            vbo == other.vbo &&
            pMesh == other.pMesh &&
            priority == other.priority;
    }

    bool RenderQueue::RenderQueueLessPredicate::operator()(const RenderQueue::MeshInfo& left, const RenderQueue::MeshInfo& right) const noexcept {
        /// Сравниваем приоритеты
        if (left.priority != right.priority) SR_UNLIKELY_ATTRIBUTE {
            return left.priority < right.priority;
        }

        /// Сравниваем указатели на шейдеры
        if (left.pShader != right.pShader) SR_LIKELY_ATTRIBUTE {
            return left.pShader < right.pShader;
        }

        /// Если шейдеры одинаковые, сравниваем VBO
        if (left.vbo != right.vbo) SR_UNLIKELY_ATTRIBUTE {
            return left.vbo < right.vbo;
        }

        /// Если и VBO одинаковые, сравниваем указатели на меши
        return left.pMesh < right.pMesh;
    }
}
