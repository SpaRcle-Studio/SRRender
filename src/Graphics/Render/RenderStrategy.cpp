//
// Created by Monika on 20.01.2024.
//

#include <Graphics/Render/RenderStrategy.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Pass/MeshDrawerPass.h>

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

    //bool RenderStrategy::IsPriorityAllowed(int64_t priority) const {
    //    return !m_priorityFilter || m_priorityFilter->IsPriorityAllowed(priority);
    //}

    //ShaderUseInfo RenderStrategy::ReplaceShader(RenderStrategy::ShaderPtr pShader) const {
    //    return m_shaderReplace ? m_shaderReplace->ReplaceShader(pShader) : ShaderUseInfo(pShader);
    //}

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

    void RenderStrategy::ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)>& callback) const {
        SR_TRACY_ZONE;

        //for (auto&& [layer, pStage] : m_layers) {
        //    pStage->ForEachMesh(callback);
        //}
    }

    void RenderStrategy::OnResourceReloaded(SR_UTILS_NS::IResource* pResource) const {
        SR_TRACY_ZONE;

        ForEachMesh([pResource](auto&& pMesh) {
            pMesh->OnResourceReloaded(pResource);
        });
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
        info.pMaterial = pMesh->GetMaterial();
        info.pShader = pMesh->GetShader();
        info.layer = pMesh->GetMeshLayer();
        info.pScene = GetRenderScene();
        info.poolId = m_meshPool.Add(pMesh);

        if (pMesh->IsSupportVBO()) {
            info.VBO = pMesh->GetVBO();
        }

        if (pMesh->HasSortingPriority()) {
            info.priority = pMesh->GetSortingPriority();
        }

        return info;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    /*MeshRenderStage::MeshRenderStage(RenderStrategy* pRenderStrategy, IRenderStage* pParent)
        : Super(pRenderStrategy, pParent)
        , m_uboManager(SR_GRAPH_NS::Memory::UBOManager::Instance())
    {
        m_meshes.reserve(64);
    }

    bool MeshRenderStage::RegisterMesh(MeshRegistrationInfo& info) {
        SR_TRACY_ZONE;
        MarkUniformsDirty();
        m_meshes.emplace_back(info.pMesh);
        info.pMeshRenderStage = this;
        return true;
    }

    bool MeshRenderStage::UnRegisterMesh(const MeshRegistrationInfo& info) {
        MarkUniformsDirty();

        if (auto&& pIt = std::find(m_meshes.begin(), m_meshes.end(), info.pMesh); pIt != m_meshes.end()) {
            m_meshes.erase(pIt);
            return true;
        }

        SRHalt("Mesh not found!");
        return false;
    }

    void MeshRenderStage::ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)>& callback) const {
        for (auto&& pMesh : m_meshes) {
            if (!pMesh) {
                continue;
            }
            callback(pMesh);
        }
    }

    bool MeshRenderStage::HasActiveMesh() const {
        SR_TRACY_ZONE;

        for (auto&& pMesh : m_meshes) {
            if (pMesh->IsMeshActive()) {
                return true;
            }
        }

        return false;
    }

    void MeshRenderStage::Update(ShaderUseInfo info) {
        SR_TRACY_ZONE;

        auto&& pMeshDrawer = m_renderStrategy->GetMeshDrawerPass();
        if (!IsNeedUpdate() && !pMeshDrawer->IsNeedUpdate()) SR_LIKELY_ATTRIBUTE {
            return;
        }

        if (!IsRendered()) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        SR_GTYPES_NS::Mesh** pMesh = m_meshes.data();
        SR_GTYPES_NS::Mesh** pMeshEnd = m_meshes.data() + m_meshes.size();

        for (; pMesh != pMeshEnd; ++pMesh) {
            auto&& pMeshUnwrapped = *pMesh;

            if (!pMeshUnwrapped->IsMeshActive()) {
                continue;
            }

            auto&& virtualUbo = pMeshUnwrapped->GetVirtualUBO();
            if (virtualUbo == SR_ID_INVALID) {
                continue;
            }

            pMeshDrawer->UseUniforms(info, pMeshUnwrapped);

            if (m_uboManager.BindUBO(virtualUbo) == Memory::UBOManager::BindResult::Duplicated) {
                SR_ERROR("VBORenderStage::Update() : memory has been duplicated!");
            }

            info.pShader->Flush();
        }
    }

    bool MeshRenderStage::Render() {
        SR_TRACY_ZONE;

        m_isRendered = true;

        for (auto&& pMesh : m_meshes) {
            pMesh->Draw();
        }

        return true;
    }

    void MeshRenderStage::MarkUniformsDirty() {
        Super::MarkUniformsDirty();
        m_parent->MarkUniformsDirty();
        m_renderStrategy->MarkUniformsDirty();
    }

    /// ----------------------------------------------------------------------------------------------------------------

    VBORenderStage::VBORenderStage(RenderStrategy* pRenderStrategy, IRenderStage* pParent, int32_t VBO)
        : Super(pRenderStrategy, pParent)
        , m_VBO(VBO)
    { }

    bool VBORenderStage::Render() {
        SR_TRACY_ZONE;

        m_isRendered = false;

        if (!IsValid()) SR_UNLIKELY_ATTRIBUTE {
            SetError("VBO is not valid!");
            return false;
        }

        if ((m_isRendered = !m_meshes.empty())) SR_LIKELY_ATTRIBUTE {
            if (!m_meshes[0]->BindMesh()) SR_UNLIKELY_ATTRIBUTE {
                return false;
            }
        }

        for (auto&& pMesh : m_meshes) {
            pMesh->Draw();
        }

        return IsRendered();
    }

    /// ----------------------------------------------------------------------------------------------------------------

    bool LayerRenderStage::Render() {
        SR_TRACY_ZONE;

        m_isRendered = false;

        for (auto&& pStage : m_priorityStages) {
            if (m_renderStrategy->IsPriorityAllowed(pStage->GetPriority())) {
                m_isRendered |= pStage->Render();
            }
        }

        for (auto&& [pShader, pShaderStage] : m_shaderStages) {
            m_isRendered |= pShaderStage->Render();
        }

        return IsRendered();
    }

    void LayerRenderStage::Update() {
        SR_TRACY_ZONE;

        if (!IsRendered()) {
            return;
        }

        for (auto&& pPriorityStage : m_priorityStages) {
            if (m_renderStrategy->IsPriorityAllowed(pPriorityStage->GetPriority())) {
                pPriorityStage->Update();
            }
        }

        for (auto&& [pShader, pShaderStage] : m_shaderStages) {
            pShaderStage->Update();
        }
    }

    bool LayerRenderStage::RegisterMesh(MeshRegistrationInfo& info) {
        SR_TRACY_ZONE;

        if (info.priority.has_value()) {
            const int64_t index = FindPriorityStageIndex(info.priority.value(), false);

            if (index == SR_ID_INVALID) {
                auto&& pStage = new PriorityRenderStage(m_renderStrategy, this, info.priority.value());
                if (!pStage->RegisterMesh(info)) {
                    delete pStage;
                    return false;
                }
                InsertPriorityStage(pStage);
                return true;
            }

            return m_priorityStages[index]->RegisterMesh(info);
        }

        if (auto&& pIt = m_shaderStages.find(info.pShader); pIt != m_shaderStages.end()) {
            return pIt->second->RegisterMesh(info);
        }
        else {
            auto&& pStage = new ShaderRenderStage(m_renderStrategy, this, info.pShader);
            if (!pStage->RegisterMesh(info)) {
                delete pStage;
                return false;
            }
            m_shaderStages[info.pShader] = pStage;
        }

        return true;
    }

    bool LayerRenderStage::UnRegisterMesh(const MeshRegistrationInfo& info) {
        if (info.priority.has_value()) {
            const int64_t index = FindPriorityStageIndex(info.priority.value(), false);
            if (index == SR_ID_INVALID) {
                SRHalt("Priority {} not found!", info.priority.value());
                return false;
            }

            const bool result = m_priorityStages[index]->UnRegisterMesh(info);

            if (m_priorityStages[index]->IsEmpty()) {
                delete m_priorityStages[index];

                memcpy(
                m_priorityStages.data() + index,
                m_priorityStages.data() + index + 1,
                ((m_priorityStages.size() - index) - 1) * sizeof(void *)
                );

                m_priorityStages.resize(m_priorityStages.size() - 1);
            }

            return result;
        }
        else {
            if (auto&& pIt = m_shaderStages.find(info.pShader); pIt != m_shaderStages.end()) {
                const bool result = pIt->second->UnRegisterMesh(info);
                if (pIt->second->IsEmpty()) {
                    delete pIt->second;
                    m_shaderStages.erase(pIt);
                }
                return result;
            }
            else {
                SRHalt("Shader not found!");
                return false;
            }
        }
        SRHalt("Unresolved situation!");
    }

    int64_t LayerRenderStage::FindPriorityStageIndex(int64_t priority, bool nearest) const {
        if (m_priorityStages.empty()) {
            return SR_ID_INVALID;
        }

        auto&& pIt = std::lower_bound(m_priorityStages.begin(), m_priorityStages.end(), priority, [](auto&& pLeft, auto&& right) -> bool {
            return pLeft->GetPriority() < right;
        });

        if (pIt == m_priorityStages.end()) {
            return SR_ID_INVALID;
        }

        if (!nearest && (*pIt)->GetPriority() != priority) {
            return SR_ID_INVALID;
        }

        return std::distance(m_priorityStages.begin(), pIt);
    }

    void LayerRenderStage::InsertPriorityStage(PriorityRenderStage* pStage) {
        if (m_priorityStages.empty()) {
            m_priorityStages.emplace_back(pStage);
            return;
        }

        const auto priority = pStage->GetPriority();

        if (priority <= m_priorityStages.front()->GetPriority()) {
            m_priorityStages.resize(m_priorityStages.size() + 1);
            memcpy(m_priorityStages.data() + 1, m_priorityStages.data(), (m_priorityStages.size() - 1) * sizeof(void*));
            m_priorityStages[0] = pStage;
            return;
        }

        if (priority >= m_priorityStages.back()->GetPriority()) {
            m_priorityStages.emplace_back(pStage);
            return;
        }

        const int64_t index = FindPriorityStageIndex(priority, true);
        if (index == SR_ID_INVALID) {
            SRHalt("Invalid index!");
            return;
        }

        m_priorityStages.resize(m_priorityStages.size() + 1);

        memcpy(
            m_priorityStages.data() + index + 1,
            m_priorityStages.data() + index,
            ((m_priorityStages.size() - index) - 1) * sizeof(void*)
        );

        m_priorityStages[index] = pStage;
    }

    void LayerRenderStage::ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)>& callback) const {
        for (auto&& pStage : m_priorityStages) {
            pStage->ForEachMesh(callback);
        }

        for (auto&& [pShader, pStage] : m_shaderStages) {
            pStage->ForEachMesh(callback);
        }
    }

    void LayerRenderStage::PostUpdate() {
        for (auto&& pStage : m_priorityStages) {
            pStage->PostUpdate();
        }

        for (auto&& [pShader, pShaderStage] : m_shaderStages) {
            pShaderStage->PostUpdate();
        }
    }

    /// ----------------------------------------------------------------------------------------------------------------

    bool PriorityRenderStage::Render() {
        SR_TRACY_ZONE;

        m_isRendered = false;

        for (auto&& [pShader, pShaderStage] : m_shaderStages) {
            m_isRendered |= pShaderStage->Render();
        }

        return IsRendered();
    }

    void PriorityRenderStage::Update() {
        SR_TRACY_ZONE;

        if (!IsRendered()) {
            return;
        }

        for (auto&& [pShader, pShaderStage] : m_shaderStages) {
            pShaderStage->Update();
        }
    }

    bool PriorityRenderStage::RegisterMesh(MeshRegistrationInfo& info) {
        SR_TRACY_ZONE;

        if (auto&& pIt = m_shaderStages.find(info.pShader); pIt != m_shaderStages.end()) {
            return pIt->second->RegisterMesh(info);
        }
        else {
            auto&& pStage = new ShaderRenderStage(m_renderStrategy, this, info.pShader);
            if (!pStage->RegisterMesh(info)) {
                delete pStage;
                return false;
            }
            m_shaderStages[info.pShader] = pStage;
        }

        return true;
    }

    bool PriorityRenderStage::UnRegisterMesh(const MeshRegistrationInfo& info) {
        if (auto&& pIt = m_shaderStages.find(info.pShader); pIt != m_shaderStages.end()) {
            const bool result = pIt->second->UnRegisterMesh(info);
            if (pIt->second->IsEmpty()) {
                delete pIt->second;
                m_shaderStages.erase(pIt);
            }
            return result;
        }

        SRHalt("Shader not found!");
        return false;
    }

    void PriorityRenderStage::ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)>& callback) const {
        for (auto&& [pShader, pStage] : m_shaderStages) {
            pStage->ForEachMesh(callback);
        }
    }

    void PriorityRenderStage::PostUpdate() {
        for (auto&& [pShader, pShaderStage] : m_shaderStages) {
            pShaderStage->PostUpdate();
        }
    }

    /// ----------------------------------------------------------------------------------------------------------------

    MaterialRenderStage::MaterialRenderStage(RenderStrategy* pRenderStrategy, IRenderStage* pParent, SR_GRAPH_NS::BaseMaterial* pMaterial)
        : Super(pRenderStrategy, pParent)
        , m_material(pMaterial)
    {
        m_meshStage = new MeshRenderStage(pRenderStrategy, this);
    }

    MaterialRenderStage::~MaterialRenderStage() {
        SRAssert(m_VBOStages.empty());
        delete m_meshStage;
    }

    bool MaterialRenderStage::Render() {
        return false;
    }

    void MaterialRenderStage::Update() {
    }

    void MaterialRenderStage::PostUpdate() {
        for (auto&& [VBO, pVBOStage] : m_VBOStages) {
            pVBOStage->PostUpdate();
        }

        if (m_meshStage) {
            m_meshStage->PostUpdate();
        }

        Super::PostUpdate();
    }

    bool MaterialRenderStage::RegisterMesh(MeshRegistrationInfo &info) {
        return IRenderStage::RegisterMesh(info);
    }

    bool MaterialRenderStage::UnRegisterMesh(const MeshRegistrationInfo &info) {
        return IRenderStage::UnRegisterMesh(info);
    }

    void MaterialRenderStage::ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)> &callback) const {
        for (auto&& [VBO, pStage] : m_VBOStages) {
            pStage->ForEachMesh(callback);
        }

        m_meshStage->ForEachMesh(callback);
    }

    bool MaterialRenderStage::HasActiveMesh() const {
        SR_TRACY_ZONE;

        if (!m_renderStrategy->IsNeedCheckMeshActivity()) {
            return true;
        }

        if (m_meshStage->HasActiveMesh()) {
            return true;
        }

        for (auto&& [VBO, pVBOStage] : m_VBOStages) {
            if (pVBOStage->HasActiveMesh()) {
                return true;
            }
        }

        return false;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    ShaderRenderStage::ShaderRenderStage(RenderStrategy* pRenderStrategy, IRenderStage* pParent, SR_GTYPES_NS::Shader* pShader)
        : Super(pRenderStrategy, pParent)
        , m_shader(pShader)
    {
        if (m_shader) {
            m_shader->AddUsePoint();
        }
        m_meshStage = new MeshRenderStage(pRenderStrategy, this);
    }

    ShaderRenderStage::~ShaderRenderStage() {
        SRAssert(m_VBOStages.empty());
        if (m_shader) {
            m_shader->RemoveUsePoint();
        }
        delete m_meshStage;
    }

    bool ShaderRenderStage::Render() {
        SR_TRACY_ZONE;

        m_isRendered = false;

        if (!IsValid()) {
            SetError("Shader is nullptr!");
            return false;
        }

        auto&& shaderUseInfo = m_renderStrategy->ReplaceShader(m_shader);
        auto&& pShader = shaderUseInfo.pShader;
        if (!pShader || !HasActiveMesh()) {
            return false;
        }

        SR_TRACY_TEXT_N("Shader", pShader->GetResourcePath().ToStringRef());

        if (pShader->Use() == ShaderBindResult::Failed) {
            return false;
        }

        if (GetRenderContext()->GetPipeline()->IsShaderChanged()) {
            auto&& pMeshPass = m_renderStrategy->GetMeshDrawerPass();
            pMeshPass->UseConstants(shaderUseInfo);
            pMeshPass->UseSamplers(shaderUseInfo);
        }

        if (!pShader->IsSamplersValid()) {
            std::string message = "Shader samplers is not valid!\n\tPath: " + pShader->GetResourcePath().ToStringRef();
            for (auto&& [name, sampler] : pShader->GetSamplers()) {
                if (GetRenderContext()->GetPipeline()->IsSamplerValid(sampler.samplerId)) {
                    continue;
                }

                message += "\n\tSampler is not set: " + name.ToStringRef();
            }
            SetError(message);
            pShader->UnUse();
            return false;
        }

        for (auto&& [VBO, pStage] : m_VBOStages) {
            m_isRendered |= pStage->Render();
        }

        m_isRendered |= m_meshStage->Render();

        pShader->UnUse();

        return IsRendered();
    }

    void ShaderRenderStage::Update() {
        SR_TRACY_ZONE;

        if (!IsRendered()) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        auto&& shaderUseInfo = m_renderStrategy->ReplaceShader(m_shader);
        auto&& pShader = shaderUseInfo.pShader;
        if (!pShader) {
            return;
        }

        if (!pShader->Ready() || !GetRenderContext()->SetCurrentShader(pShader)) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        if (SR_TRACY_IS_PROFILER_CONNECTED) {
            auto&& path = pShader->GetResourcePath().ToStringRef();
            SR_TRACY_TEXT_N("Shader", path);
        }

        auto&& pMeshDrawer = m_renderStrategy->GetMeshDrawerPass();

        if (pShader->BeginSharedUBO()) {
            pMeshDrawer->UseSharedUniforms(shaderUseInfo);
            pShader->EndSharedUBO();
        }

        if (IsNeedUpdate() || pMeshDrawer->IsNeedUpdate()) {
            for (auto&& [VBO, pVBOStage] : m_VBOStages) {
                pVBOStage->Update(shaderUseInfo);
            }

            m_meshStage->Update(shaderUseInfo);
        }

        GetRenderScene()->SetCurrentSkeleton(nullptr);
        GetRenderContext()->SetCurrentShader(nullptr);
    }

    bool ShaderRenderStage::HasActiveMesh() const {
        SR_TRACY_ZONE;

        if (!m_renderStrategy->IsNeedCheckMeshActivity()) {
            return true;
        }

        if (m_meshStage->HasActiveMesh()) {
            return true;
        }

        for (auto&& [VBO, pVBOStage] : m_VBOStages) {
            if (pVBOStage->HasActiveMesh()) {
                return true;
            }
        }

        return false;
    }

    bool ShaderRenderStage::RegisterMesh(MeshRegistrationInfo& info) {
        SR_TRACY_ZONE;

        if (!info.VBO.has_value()) {
            return m_meshStage->RegisterMesh(info);
        }

        if (auto&& pIt = m_VBOStages.find(info.VBO.value()); pIt != m_VBOStages.end()) {
            return pIt->second->RegisterMesh(info);
        }
        else {
            auto&& pStage = new VBORenderStage(m_renderStrategy, this, info.VBO.value());
            if (!pStage->RegisterMesh(info)) {
                delete pStage;
                return false;
            }
            m_VBOStages[info.VBO.value()] = pStage;
        }

        return true;
    }

    bool ShaderRenderStage::UnRegisterMesh(const MeshRegistrationInfo& info) {
        SR_TRACY_ZONE;

        if (!info.VBO.has_value()) {
            return m_meshStage->UnRegisterMesh(info);
        }

        if (auto&& pIt = m_VBOStages.find(info.VBO.value()); pIt != m_VBOStages.end()) {
            const bool result = pIt->second->UnRegisterMesh(info);
            if (pIt->second->IsEmpty()) {
                delete pIt->second;
                m_VBOStages.erase(pIt);
            }
            return result;
        }

        SRHalt("VBO {} not found!", info.VBO.value());
        return false;
    }

    void ShaderRenderStage::ForEachMesh(const SR_HTYPES_NS::Function<void(MeshPtr)>& callback) const {
        for (auto&& [VBO, pStage] : m_VBOStages) {
            pStage->ForEachMesh(callback);
        }

        m_meshStage->ForEachMesh(callback);
    }

    void ShaderRenderStage::PostUpdate() {
        for (auto&& [VBO, pVBOStage] : m_VBOStages) {
            pVBOStage->PostUpdate();
        }

        if (m_meshStage) {
            m_meshStage->PostUpdate();
        }

        Super::PostUpdate();
    }

    /// ----------------------------------------------------------------------------------------------------------------

    IRenderStage::IRenderStage(RenderStrategy *pRenderStrategy, IRenderStage *pParent)
        : Super()
        , m_parent(pParent)
        , m_renderStrategy(pRenderStrategy)
        , m_renderContext(pRenderStrategy->GetRenderContext())
    { }

    RenderContext* IRenderStage::GetRenderContext() const {
        return m_renderStrategy->GetRenderContext();
    }

    RenderScene* IRenderStage::GetRenderScene() const {
        return m_renderStrategy->GetRenderScene();
    }

    void IRenderStage::SetError(SR_UTILS_NS::StringAtom error) {
        m_renderStrategy->AddError(error);

        if (m_renderStrategy->IsDebugModeEnabled()) {
            ForEachMesh([this](auto&& pMesh) {
                if (!pMesh->IsMeshActive()) {
                    return;
                }
                m_renderStrategy->AddProblemMesh(pMesh);
            });
        }
    }

    bool IRenderStage::IsNeedUpdate() const {
        return m_uniformsDirty || !m_renderContext->IsOptimizedRenderUpdateEnabled();
    }*/
}