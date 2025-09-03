//
// Created by Nikita on 17.11.2020.
//

#include <Utils/ECS/Component.h>
#include <Utils/Resources/ResourceManager.h>
#include <Utils/Resources/IResource.h>
#include <Utils/ECS/Node.h>

#include <Graphics/Types/Mesh.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderStrategy.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Material/FileMaterial.h>

#include <Codegen/Mesh.generated.hpp>

namespace SR_GTYPES_NS {
    Mesh::Mesh()
        : m_uboManager(Memory::UBOManager::Instance())
        , m_descriptorManager(SR_GRAPH_NS::DescriptorManager::Instance())
    { }

    Mesh::~Mesh() {
        SRAssert(m_isDestroyingState);
        SRAssert(m_virtualUBO == SR_ID_INVALID);
        SRAssert(!m_registrationInfo.has_value());
        SRAssert2(!m_isUniformsDirty, "Application will crash if you delete mesh with dirty uniforms!");
        SRAssert2(!m_isWaitReRegister, "Application may will crash if you delete mesh with waiting re-register!");
    }

    Mesh::Ptr Mesh::Load(const SR_UTILS_NS::Path& path, MeshType type, uint32_t id) {
        if (auto&& pRawMesh =  SR_HTYPES_NS::RawMesh::Load(path)) {
            return TryLoad(pRawMesh.Get(), type, id);
        }

        SR_ERROR("Mesh::Load() : failed to load mesh!\n\tPath: " + path.ToStringRef() + "\n\tId: " + std::to_string(id));

        return nullptr;
    }

    Mesh::Ptr Mesh::TryLoad(const SR_UTILS_NS::Path &path, MeshType type, uint32_t id) {
        if (auto&& pRawMesh = SR_HTYPES_NS::RawMesh::Load(path)) {
            if (auto&& pMesh = TryLoad(pRawMesh.Get(), type, id)) {
                return pMesh;
            }
            pRawMesh->CheckResourceUsage();
        }
        return nullptr;
    }

    Mesh::Ptr Mesh::TryLoad(SR_HTYPES_NS::RawMesh* pRawMesh, MeshType type, uint32_t id) {
        Mesh::Ptr pMesh = nullptr;
        bool exists = false;

        /// Проверяем существование меша
        if (pRawMesh) {
            exists = id < pRawMesh->GetMeshesCount();
        }
        else {
            SRHalt("Mesh::TryLoad() : raw mesh is nullptr!");
            return nullptr;
        }

        if (!exists || !((pMesh = CreateMeshByType(type)))) {
            pRawMesh->CheckResourceUsage();
            return nullptr;
        }

        if (auto&& pRawMeshHolder = pMesh.DynamicCast<SR_HTYPES_NS::IRawMeshHolder>()) {
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

    void Mesh::SetMatrix(const SR_MATH_NS::Matrix4x4& /* matrix */) {
        MarkUniformsDirty();
    }

    const SR_MATH_NS::Matrix4x4& Mesh::GetMatrix() const {
        if (HasParent()) SR_LIKELY_ATTRIBUTE {
            auto&& pSO = GetSceneObject();
            switch (pSO->GetSceneObjectType()) {
                case SR_UTILS_NS::SceneObjectType::GameObject:
                    return static_cast<const SR_UTILS_NS::GameObject*>(pSO.Get())->GetTransform()->GetMatrix();
                case SR_UTILS_NS::SceneObjectType::Node:
                    return static_cast<const SR_UTILS_NS::Node*>(pSO.Get())->GetMatrix();
                default:
                    SRHalt("Mesh::GetMatrix() : unknown scene object type!");
                    break;
            }
        }

        static SR_MATH_NS::Matrix4x4 identity = SR_MATH_NS::Matrix4x4::Identity();
        return identity;
    }

    std::vector<Mesh::Ptr> Mesh::Load(const SR_UTILS_NS::Path& path, MeshType type) {
        std::vector<Mesh::Ptr> meshes;

        uint32_t id = 0;
        auto&& pRawMesh = SR_HTYPES_NS::RawMesh::Load(path);
        while (pRawMesh) {
            if (auto&& pMesh = TryLoad(pRawMesh.Get(), type, id)) {
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
        if (m_virtualUBO != SR_ID_INVALID && !m_uboManager.FreeUBO(&m_virtualUBO)) {
            SR_ERROR("Mesh::FreeVideoMemory() : failed to free virtual uniform buffer object!");
        }

        if (m_virtualDescriptor != SR_ID_INVALID) {
            m_descriptorManager.FreeDescriptorSet(&m_virtualDescriptor);
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

        if (m_material) {
            m_material->UnregisterMesh(&m_materialRegisterId);
        }

        m_material = pMaterial;

        if (m_material) {
            m_materialRegisterId = m_material->RegisterMesh(this);
        }

        if (m_registrationInfo.has_value()) {
            ReRegisterMesh();
        }
    }

    //Mesh::ShaderPtr Mesh::GetShader() const {
    //    return m_material ? m_material->GetShader() : nullptr;
    //}

    void Mesh::UseMaterial() {
        SR_TRACY_ZONE;

        if (m_material) {
            m_material->Use();
        }
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

        SRAssert(IRenderComponent::IsActive());

        if (m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
            m_virtualUBO = m_uboManager.AllocateUBO(m_virtualUBO);
            if (m_virtualUBO == SR_INVALID_UBO) SR_UNLIKELY_ATTRIBUTE {
                m_hasErrors = true;
                return;
            }

            m_virtualDescriptor = m_descriptorManager.AllocateDescriptorSet(m_virtualDescriptor);
        }

        SRAssert(m_virtualUBO != SR_INVALID_UBO);

        m_uboManager.BindUBO(m_virtualUBO);

        const auto result = m_descriptorManager.Bind(m_virtualDescriptor);

        if (result == DescriptorManager::BindResult::Duplicated || m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
            UseSamplers();
            UseSSBO();
            MarkUniformsDirty(true);
            m_descriptorManager.Flush();
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

    void Mesh::UseSamplers() {
        if (m_material) {
            m_material->UseSamplers();
        }
    }

    std::string Mesh::GetMeshIdentifier() const {
        static const std::string empty;
        return empty;
    }

    int64_t Mesh::GetSortingPriority() const {
        if (auto&& pSO = GetSceneObject()) {
            if (pSO->GetSceneObjectType() == SR_UTILS_NS::SceneObjectType::Node) {
                return static_cast<const SR_UTILS_NS::Node*>(pSO.Get())->GetNodePriority();
            }
        }
        return -1;
    }

    bool Mesh::HasSortingPriority() const {
        if (auto&& pSO = GetSceneObject()) {
            return pSO->GetSceneObjectType() == SR_UTILS_NS::SceneObjectType::Node;
        }
        return false;
    }

    SR_UTILS_NS::StringAtom Mesh::GetMeshLayer() const {
        if (!m_sceneObject) {
            return SR_UTILS_NS::StringAtom();
        }

        return m_sceneObject->GetLayer();
    }

    bool Mesh::OnResourceReloaded(const SR_UTILS_NS::IResource* pResource) {
        return false;
    }

    void Mesh::MarkMaterialDirty() {
        m_dirtyMaterial = true;
    }

    Mesh::Ptr Mesh::Load(const SR_UTILS_NS::Path& path, MeshType type, SR_UTILS_NS::StringAtom name) {
        if (auto&& pRawMesh = SR_HTYPES_NS::RawMesh::Load(path)) {
            return Load(path, type, pRawMesh->GetMeshId(name));
        }
        return nullptr;
    }

    void Mesh::OnDestroy() {
        DestroyMesh();
        Super::OnDestroy();
    }

    void Mesh::OnMatrixDirty() {
        MarkUniformsDirty();
        Super::OnMatrixDirty();
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

    void Mesh::UnRegisterMesh() {
        if (IsMeshRegistered()) {
            m_registrationInfo.value().pScene->Remove(this);
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

        if (m_registrationInfo.has_value()) {
            if (m_isWaitReRegister) {
                return;
            }

            m_isWaitReRegister = true;

            const auto pRenderScene = m_registrationInfo.value().pScene;
            pRenderScene->ReRegister(m_registrationInfo.value());
        }
        else if (auto&& pRenderScene = TryGetRenderScene()) {
            pRenderScene->Register(this);
        }
    }

    bool Mesh::DestroyMesh() {
        m_isDestroyingState = true;

        const bool isRegistered = IsMeshRegistered();
        if (isRegistered) {
            m_registrationInfo.value().pScene->Remove(this);
        }

        FreeVMemory();
        SetMaterial(MaterialPtr());

        return isRegistered;
    }

    void Mesh::OnReRegistered() {
        m_isWaitReRegister = false;
    }

    void Mesh::MarkUniformsDirty(bool force) {
        if (m_isUniformsDirty && !force) SR_LIKELY_ATTRIBUTE {
            return;
        }

        SR_TRACY_ZONE;

        if (!IsActive()) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        m_isUniformsDirty = !m_renderQueues.empty();

        auto pStart = m_renderQueues.data();
        auto pEnd = pStart + m_renderQueues.size();
        for (auto pElement = pStart; pElement != pEnd; ++pElement) {
            pElement->pRenderQueue->OnMeshDirty(this, pElement->pShader);
        }
    }
}

