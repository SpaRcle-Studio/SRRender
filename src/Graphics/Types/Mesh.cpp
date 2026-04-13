//
// Created by Nikita on 17.11.2020.
//

#include <Graphics/Types/Mesh.h>
#include <Graphics/Types/Geometry/SkinnedMesh.h>
#include <Graphics/Types/Geometry/Mesh3D.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderStrategy.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Material/FileMaterial.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Memory/DescriptorManager.h>

#include <Utils/ECS/TransformRect.h>
#include <Utils/ECS/GameObject.h>
#include <Utils/Resources/IResource.h>
#include <Utils/Types/RawMesh.h>
#include <Utils/Types/IRawMeshHolder.h>

#include <Codegen/Mesh.generated.hpp>

namespace SR_GTYPES_NS {
    Mesh::Mesh()
        : Super()
    { }

    Mesh::~Mesh() {
        SRAssert(m_isDestroyingState || !m_isCalculated);
        SRAssert(m_virtualUBO == SR_ID_INVALID);
        SRAssert(!GetInternalData().registrationInfo.has_value());
        SRAssert2(!m_isWaitReRegister, "Application may will crash if you delete mesh with waiting re-register!");
    }

    Mesh::Ptr Mesh::Load(const SR_UTILS_NS::Path& path, uint32_t id) {
        if (auto&& pRawMesh =  CoreResLoader::Load<SR_HTYPES_NS::RawMesh>(path)) {
            return TryLoad(pRawMesh.Get(), id);
        }

        SR_ERROR("Mesh::Load() : failed to load mesh!\n\tPath: " + path.ToStringRef() + "\n\tId: " + std::to_string(id));

        return nullptr;
    }

    Mesh::Ptr Mesh::TryLoad(const SR_UTILS_NS::Path& path, uint32_t id) {
        if (auto&& pRawMesh = CoreResLoader::Load<SR_HTYPES_NS::RawMesh>(path)) {
            if (auto&& pMesh = TryLoad(pRawMesh.Get(), id)) {
                return pMesh;
            }
            pRawMesh->CheckResourceUsage();
        }
        return nullptr;
    }

    Mesh::Ptr Mesh::TryLoad(SR_HTYPES_NS::RawMesh* pRawMesh, uint32_t id) {
        /// Проверяем существование меша
        if (pRawMesh) {
            if (id >= pRawMesh->GetMeshesCount()) {
                pRawMesh->CheckResourceUsage();
                return nullptr;
            }
        }
        else {
            SRHalt("Mesh::TryLoad() : raw mesh is nullptr!");
            return nullptr;
        }

        Mesh::Ptr pMesh = pRawMesh->HasBones(id)
            ? SR_UTILS_NS::Factory::Instance().Create<SkinnedMesh>().StaticCast<Mesh>()
            : SR_UTILS_NS::Factory::Instance().Create<Mesh3D>().StaticCast<Mesh>();

        if (auto&& pRawMeshHolder = SR_UTILS_NS::DynamicPointerCast<SR_HTYPES_NS::IRawMeshHolder>(pMesh)) {
            pRawMeshHolder->SetRawMesh(pRawMesh);
            pRawMeshHolder->SetMeshId(id);
        }
        else {
            SRHalt("Mesh is not a raw mesh holder! Memory leak possible...");
            pRawMesh->CheckResourceUsage();
            return nullptr;
        }

        return pMesh;
    }

    const SR_MATH_NS::Matrix4x4& Mesh::GetMatrix() const {
        if (HasParent()) SR_LIKELY_ATTRIBUTE {
            auto&& pSO = GetSceneObject();
            switch (pSO->GetSceneObjectType()) {
                case SR_UTILS_NS::SceneObjectType::GameObject:
                    return static_cast<const SR_UTILS_NS::GameObject*>(pSO.Get())->GetTransform()->GetMatrix();
                //case SR_UTILS_NS::SceneObjectType::Node:
                //    return static_cast<const SR_UTILS_NS::Node*>(pSO.Get())->GetMatrix();
                default:
                    SRHalt("Mesh::GetMatrix() : unknown scene object type!");
                    break;
            }
        }

        static SR_MATH_NS::Matrix4x4 identity = SR_MATH_NS::Matrix4x4::Identity();
        return identity;
    }

    std::vector<Mesh::Ptr> Mesh::Load(const SR_UTILS_NS::Path& path) {
        std::vector<Mesh::Ptr> meshes;

        uint32_t id = 0;
        auto&& pRawMesh = CoreResLoader::Load<SR_HTYPES_NS::RawMesh>(path);
        while (pRawMesh) {
            if (auto&& pMesh = TryLoad(pRawMesh.Get(), id)) {
                meshes.emplace_back(pMesh);
                ++id;
            }
            else {
                break;
            }
        }

        if (meshes.empty()) {
            SR_ERROR("Mesh::Load() : failed to load mesh! Path: " + path.ToString());
        }

        return meshes;
    }

    bool Mesh::IsCalculatable() const {
        return true;
    }

    bool Mesh::IsActive() const noexcept {
        return IRenderComponent::IsActive() && !m_hasErrors;
    }

    void Mesh::FreeVMemory() {
        if (m_pipeline) {
            m_pipeline->GetUBOManager().TryFreeUBO(&m_virtualUBO);
            m_pipeline->GetDescriptorManager().TryFreeDescriptorSet(&m_virtualDescriptor);
        }
    }

    bool Mesh::Calculate() {
        m_isCalculated = true;
        /// чтобы в случае перезагрузки обновить все связанные данные
        MarkMaterialDirty();
        return true;
    }

    void Mesh::SetMaterial(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;
        SetMaterial(FileMaterial::Load(path));
    }

    void Mesh::SetMaterial(const MaterialPtr& pMaterial) {
        SR_TRACY_ZONE;

        if (pMaterial == m_material) {
            return;
        }

        MarkMaterialDirty();
        SetErrorsClean();

        auto&& internalData = GetInternalData();
        if (m_material) {
            m_material->UnregisterMesh(&internalData.materialRegisterId);
        }

        m_material = pMaterial;

        if (m_material) {
            internalData.materialRegisterId = m_material->RegisterMesh(this);
        }

        if (internalData.registrationInfo.has_value()) {
            ReRegisterMesh();
        }
    }

    void Mesh::UseMaterial(SR_GTYPES_NS::Shader& shader) {
        m_material->Use(shader);
    }

    bool Mesh::BindMesh() {
        SR_TRACY_ZONE;

        if (auto&& VBO = GetVBO(); VBO != SR_ID_INVALID) SR_LIKELY_ATTRIBUTE {
            GetPipeline()->BindVBO(VBO);
        }
        else {
            return false;
        }

        if (auto&& IBO = GetIBO(); IBO != SR_ID_INVALID) SR_LIKELY_ATTRIBUTE {
            GetPipeline()->BindIBO(IBO);
        }
        else {
            return false;
        }

        return true;
    }

    void Mesh::Draw() {
        SR_TRACY_ZONE;

        if (!Calculate() || m_hasErrors) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        SRAssert(m_pipeline && IRenderComponent::IsActive());

        if (m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
            m_virtualUBO = m_pipeline->GetUBOManager().AllocateUBO(m_virtualUBO);
            if (m_virtualUBO == SR_INVALID_UBO) SR_UNLIKELY_ATTRIBUTE {
                m_hasErrors = true;
                return;
            }

            m_virtualDescriptor = m_pipeline->GetDescriptorManager().AllocateDescriptorSet(m_virtualDescriptor);
        }

        SRAssert(m_virtualUBO != SR_INVALID_UBO);

        m_pipeline->GetUBOManager().BindUBO(m_virtualUBO);

        const auto result = m_pipeline->GetDescriptorManager().Bind(m_virtualDescriptor);

        if (result == DescriptorManager::BindResult::Duplicated || m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
            UseSamplers(*m_pipeline->GetCurrentShader());
            UseSSBO();
            MarkUniformsDirty();
            m_pipeline->GetDescriptorManager().Flush();
        }
        GetPipeline()->GetCurrentShader()->FlushConstants();

        if (result != DescriptorManager::BindResult::Failed) SR_UNLIKELY_ATTRIBUTE {
            if (IsSupportVBO()) {
                GetPipeline()->DrawIndices(GetIndicesCount());
            }
            else {
                GetPipeline()->Draw(GetIndicesCount());
            }
        }

        m_dirtyMaterial = false;
    }

    void Mesh::UseSamplers(SR_GTYPES_NS::Shader& shader) {
        SR_TRACY_ZONE;
        m_material->UseSamplers(shader);
    }

    int64_t Mesh::GetSortingPriority() const {
        if (auto&& pSO = GetSceneObject()) {
            //if (pSO->GetSceneObjectType() == SR_UTILS_NS::SceneObjectType::Node) {
            //    return static_cast<const SR_UTILS_NS::Node*>(pSO.Get())->GetNodePriority();
            //}
            if (pSO->GetSceneObjectType() == SR_UTILS_NS::SceneObjectType::GameObject) {
                auto&& pTransform = static_cast<const SR_UTILS_NS::GameObject*>(pSO.Get())->GetTransform();
                if (pTransform->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D) {
                    return static_cast<const SR_UTILS_NS::TransformRect*>(pTransform.Get())->GetPriority();
                }
            }
        }
        return -1;
    }

    bool Mesh::HasSortingPriority() const {
        if (auto&& pSO = GetSceneObject()) {
            if (pSO->GetSceneObjectType() == SR_UTILS_NS::SceneObjectType::GameObject) {
                return static_cast<const SR_UTILS_NS::GameObject*>(pSO.Get())->GetTransform()->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D;
            }
        }
        return false;
    }

    SR_UTILS_NS::StringAtom Mesh::GetMeshLayer() const {
        if (auto&& pSO = GetSceneObject()) {
            return pSO->GetLayer();
        }
        return SR_UTILS_NS::StringAtom();
    }

    bool Mesh::OnResourceReloaded(SR_UTILS_NS::StringAtom resourceId) {
        return false;
    }

    void Mesh::MarkMaterialDirty() {
        m_dirtyMaterial = true;
    }

    Mesh::Ptr Mesh::Load(const SR_UTILS_NS::Path& path, SR_UTILS_NS::StringAtom name) {
        if (auto&& pRawMesh = CoreResLoader::Load<SR_HTYPES_NS::RawMesh>(path)) {
            return Load(path, pRawMesh->GetMeshId(name));
        }
        return nullptr;
    }

    void Mesh::OnDestroy() {
        DestroyMesh();
        Super::OnDestroy();
    }

    void Mesh::OnMaskDirty() {
        Super::OnMaskDirty();
        m_isMaskDirty = true;
    }

    void Mesh::OnMatrixDirty() {
        MarkUniformsDirty();
        Super::OnMatrixDirty();
        m_isAABBDirty = true;
    }

    void Mesh::OnLayerChanged() {
        ReRegisterMesh();
        Super::OnLayerChanged();
    }

    void Mesh::OnPriorityChanged() {
        ReRegisterMesh();
        Super::OnPriorityChanged();
    }

    void Mesh::OnEnable() {
        Super::OnEnable();
        if (!IsMeshRegistered()) {
            ReRegisterMesh();
        }
        MarkUniformsDirty();
    }

    void Mesh::OnDisable() {
        Super::OnDisable();
        UnRegisterMesh();
    }

    const SR_UTILS_NS::UI::MaskInfo& Mesh::GetMaskInfo() const {
        if (m_isMaskDirty) {
            if (auto&& pTransform = GetTransformAs<SR_UTILS_NS::TransformRect>()) {
                m_maskInfo = pTransform->GetMaskInfo();
                m_isMaskDirty = false;
            }
        }
        return m_maskInfo;
    }

    const SR_MATH_NS::AABB& Mesh::GetAABB() const {
        auto&& internalData = GetInternalData();
        if (m_isAABBDirty) {
            if (auto&& pTransform = GetTransform()) {
                internalData.aabb = pTransform->GetAABB();
                m_isAABBDirty = false;
            }
        }
        return internalData.aabb;
    }

    void Mesh::UnRegisterMesh() {
        if (IsMeshRegistered()) {
            //GetInternalData().registrationInfo.value().pScene->Remove(this);
            GetRenderScene()->Remove(this);
        }
    }

    void Mesh::ReRegisterMesh() {
        SR_TRACY_ZONE;

        if (!IsActive()) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        if (m_isDestroyingState) {
            return;
        }

        auto&& pRenderScene = TryGetRenderScene();
        auto&& internalData = GetInternalData();
        if (internalData.registrationInfo.has_value()) {
            if (m_isWaitReRegister) {
                return;
            }

            m_isWaitReRegister = true;

            //const auto pRenderScene = internalData.registrationInfo.value().pScene;
            pRenderScene->ReRegister(internalData.registrationInfo.value());
        }
        else if (pRenderScene) {
            pRenderScene->Register(this);
        }
    }

    bool Mesh::DestroyMesh() {
        m_isDestroyingState = true;

        const bool isRegistered = IsMeshRegistered();
        if (isRegistered) {
            //GetInternalData().registrationInfo.value().pScene->Remove(this);
            GetRenderScene()->Remove(this);
        }

        FreeVMemory();
        SetMaterial(MaterialPtr());

        return isRegistered;
    }

    void Mesh::OnReRegistered() {
        m_isWaitReRegister = false;
    }

    void Mesh::MarkUniformsDirty() {
        SR_TRACY_ZONE;

        if (!IsActive()) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        auto&& renderQueues = GetRenderQueues();
        auto pStart = renderQueues.data();
        auto pEnd = pStart + renderQueues.size();
        for (auto pElement = pStart; pElement != pEnd; ++pElement) {
            if (!pElement->pRenderQueue) SR_LIKELY_ATTRIBUTE {
                continue;
            }

            if (!pElement->isVisible) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            pElement->dirtyUniformsFrames.set();
            if (pElement->inUpdateQueue) {
                continue;
            }
            pElement->inUpdateQueue = true;
            pElement->pRenderQueue->OnMeshDirty(this, pElement);
        }
    }

    const SR_UTILS_NS::VertexLayoutDescription& Mesh::GetVertexLayoutDescription() const noexcept {
        return GetInternalData().vertexLayoutDescription;
    }

    void Mesh::SetVertexLayoutDescription(const SR_UTILS_NS::VertexLayoutDescription& description) {
        GetInternalData().vertexLayoutDescription = description;
        m_isCalculated = false;
    }

    Mesh::RenderQueues& Mesh::GetRenderQueues() noexcept {
        return GetInternalData().renderQueues;
    }

    const SR_UTILS_NS::VertexLayoutDescription& Mesh::GetShaderVertexLayoutDescription() const noexcept {
        static SR_UTILS_NS::VertexLayoutDescription emptyDescription;
        return emptyDescription;
    }

    void Mesh::SetMeshRegistrationInfo(const std::optional<MeshRegistrationInfo>& info) {
        GetInternalData().registrationInfo = info;
    }

    Details::MeshInternalData& Mesh::GetInternalData() const {
        if (!m_pInternalData) {
            m_pInternalData = new Details::MeshInternalData();
        }
        return *m_pInternalData;
    }

    bool Mesh::IsMeshRegistered() const noexcept {
        return GetInternalData().registrationInfo.has_value();
    }

    const MeshRegistrationInfo& Mesh::GetMeshRegistrationInfo() const noexcept {
        return GetInternalData().registrationInfo.value();
    }
}

