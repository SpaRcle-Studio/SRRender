//
// Created by Nikita on 17.11.2020.
//

#include <Utils/ECS/Component.h>
#include <Utils/Resources/ResourceManager.h>
#include <Utils/Resources/IResource.h>

#include <Graphics/Types/Mesh.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Render/RenderStrategy.h>
#include <Graphics/Render/RenderQueue.h>
#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Material/FileMaterial.h>

namespace SR_GTYPES_NS {
    Mesh::Mesh(MeshType type)
        : m_uboManager(Memory::UBOManager::Instance())
        , m_descriptorManager(SR_GRAPH_NS::DescriptorManager::Instance())
        , m_meshType(type)
    {
        m_materialProperty.SetMesh(this);
    }

    Mesh::~Mesh() {
        SetMaterial(nullptr);
        SRAssert(m_virtualUBO == SR_ID_INVALID);
        SRAssert(!m_registrationInfo.has_value());
    }

    Mesh::Ptr Mesh::Load(const SR_UTILS_NS::Path& path, MeshType type, uint32_t id) {
        if (auto&& pRawMesh =  SR_HTYPES_NS::RawMesh::Load(path)) {
            return TryLoad(pRawMesh, type, id);
        }

        SR_ERROR("Mesh::Load() : failed to load mesh!\n\tPath: " + path.ToStringRef() + "\n\tId: " + std::to_string(id));

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

        if (!exists || !(pMesh = CreateMeshByType(type))) {
            pRawMesh->CheckResourceUsage();
            return nullptr;
        }

        if (auto&& pRawMeshHolder = dynamic_cast<SR_HTYPES_NS::IRawMeshHolder*>(pMesh)) {
            pRawMeshHolder->SetRawMesh(pRawMesh);
            pRawMeshHolder->SetMeshId(id);
        }
        else {
            SRHalt("Mesh is not a raw mesh holder! Memory leak...");
            pRawMesh->CheckResourceUsage();
            return nullptr;
        }

        return pMesh;
    }

    std::vector<Mesh::Ptr> Mesh::Load(const SR_UTILS_NS::Path& path, MeshType type) {
        std::vector<Mesh::Ptr> meshes;

        uint32_t id = 0;
        while (auto&& pRawMesh = SR_HTYPES_NS::RawMesh::Load(path)) {
            if (auto&& pMesh = TryLoad(pRawMesh, type, id)) {
                meshes.emplace_back(pMesh);
                ++id;
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

    void Mesh::FreeVideoMemory() {
        if (m_virtualUBO != SR_ID_INVALID && !m_uboManager.FreeUBO(&m_virtualUBO)) {
            SR_ERROR("Mesh::FreeVideoMemory() : failed to free virtual uniform buffer object!");
        }

        if (m_virtualDescriptor != SR_ID_INVALID) {
            m_descriptorManager.FreeDescriptorSet(&m_virtualDescriptor);
        }

        IGraphicsResource::FreeVideoMemory();
    }

    bool Mesh::Calculate() {
        m_isCalculated = true;
        /// чтобы в случае перезагрузки обновить все связанные данные
        MarkMaterialDirty();
        return true;
    }

    void Mesh::SetMaterial(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;
        m_materialProperty.SetMaterial(path);
    }

    void Mesh::SetMaterial(MaterialPtr pMaterial) {
        SR_TRACY_ZONE;
        m_materialProperty.SetMaterial(pMaterial);
    }

    Mesh::ShaderPtr Mesh::GetShader() const {
        if (auto&& pMaterial = m_materialProperty.GetMaterial()) {
            return pMaterial->GetShader();
        }
        return nullptr;
    }

    void Mesh::UseOverrideUniforms() {
        SR_TRACY_ZONE;

        for (auto&& property : m_overrideUniforms) {
            if (!property.IsSampler()) {
                property.Use(GetRenderContext()->GetCurrentShader());
            }
        }
    }

    void Mesh::UseMaterial() {
        SR_TRACY_ZONE;
        if (auto&& pMaterial = m_materialProperty.GetMaterial()) {
            pMaterial->Use();
        }
    }

    bool Mesh::BindMesh() {
        SR_TRACY_ZONE;

        if (auto&& VBO = GetVBO(); VBO != SR_ID_INVALID) SR_LIKELY_ATTRIBUTE {
            m_pipeline->BindVBO(VBO);
        }
        else {
            return false;
        }

        if (auto&& IBO = GetIBO(); IBO != SR_ID_INVALID) SR_LIKELY_ATTRIBUTE {
            m_pipeline->BindIBO(IBO);
        }
        else {
            return false;
        }

        return true;
    }

    const SR_MATH_NS::Matrix4x4& Mesh::GetModelMatrix() const {
        static SR_MATH_NS::Matrix4x4 matrix4X4 = SR_MATH_NS::Matrix4x4::Identity();
        return matrix4X4;
    }

    void Mesh::UseSamplers() {
        if (auto&& pMaterial = m_materialProperty.GetMaterial()) {
            pMaterial->UseSamplers();
        }
    }

    std::string Mesh::GetMeshIdentifier() const {
        static const std::string empty;
        return empty;
    }

    bool Mesh::OnResourceReloaded(SR_UTILS_NS::IResource* pResource) {
        return false;
    }

    void Mesh::MarkMaterialDirty() {
        m_dirtyMaterial = true;
        MarkUniformsDirty();
    }

    Mesh::Ptr Mesh::TryLoad(const SR_UTILS_NS::Path &path, MeshType type, uint32_t id) {
        Mesh::Ptr pMesh = nullptr;
        bool exists = false;

        /// Проверяем существование меша
        SR_HTYPES_NS::RawMesh* pRawMesh = nullptr;
        if ((pRawMesh = SR_HTYPES_NS::RawMesh::Load(path))) {
            exists = id < pRawMesh->GetMeshesCount();
        }
        else {
            return nullptr;
        }

        if (!exists || !(pMesh = CreateMeshByType(type))) {
            pRawMesh->CheckResourceUsage();
            return nullptr;
        }

        if (auto&& pRawMeshHolder = dynamic_cast<SR_HTYPES_NS::IRawMeshHolder*>(pMesh)) {
            pRawMeshHolder->SetRawMesh(pRawMesh);
            pRawMeshHolder->SetMeshId(id);
        }
        else {
            SRHalt("Mesh is not a raw mesh holder! Memory leak...");
            pRawMesh->CheckResourceUsage();
            return nullptr;
        }

        return pMesh;
    }

    Mesh::Ptr Mesh::Load(const SR_UTILS_NS::Path& path, MeshType type, SR_UTILS_NS::StringAtom name) {
        if (auto&& pRawMesh = SR_HTYPES_NS::RawMesh::Load(path)) {
            return Load(path, type, pRawMesh->GetMeshId(name));
        }
        return nullptr;
    }

    void Mesh::UnRegisterMesh() {
        if (IsMeshRegistered()) {
            m_registrationInfo.value().pScene->Remove(this);
        }
    }

    void Mesh::ReRegisterMesh() {
        SR_TRACY_ZONE;
        if (m_registrationInfo.has_value()) {
            const auto pRenderScene = m_registrationInfo.value().pScene;
            pRenderScene->ReRegister(m_registrationInfo.value());
        }
    }

    bool Mesh::DestroyMesh() {
        const bool isRegistered = IsMeshRegistered();
        if (isRegistered) {
            m_registrationInfo.value().pScene->Remove(this);
        }

        if (IsCalculated()) {
            FreeVideoMemory();
            DeInitGraphicsResource();
        }

        if (auto&& pRenderComponent = dynamic_cast<IRenderComponent*>(this)) {
            pRenderComponent->AutoFree();
        }
        else {
            delete this;
        }

        return isRegistered;
    }

    MaterialProperty& Mesh::OverrideUniform(SR_UTILS_NS::StringAtom name) {
        SR_TRACY_ZONE;

        for (auto&& uniform : m_overrideUniforms) {
            if (uniform.GetName() == name) {
                return uniform;
            }
        }

        m_overrideUniforms.emplace_back();
        m_overrideUniforms.back().SetName(name);
        return m_overrideUniforms.back();
    }

    void Mesh::RemoveUniformOverride(SR_UTILS_NS::StringAtom name) {
        SR_TRACY_ZONE;

        for (auto pIt = m_overrideUniforms.begin(); pIt != m_overrideUniforms.end(); ++pIt) {
            if (name == pIt->GetName()) {
                m_overrideUniforms.erase(pIt);
                return;
            }
        }
    }

    MaterialProperty& Mesh::OverrideConstant(SR_UTILS_NS::StringAtom name) {
        SR_TRACY_ZONE;

        if (auto&& pPipeline = GetPipeline()) {
            pPipeline->SetDirty(true);
        }

        for (auto&& constant : m_overrideConstant) {
            if (constant.GetName() == name) {
                return constant;
            }
        }
        m_overrideConstant.emplace_back();
        m_overrideConstant.back().SetName(name);
        return m_overrideConstant.back();
    }

    void Mesh::RemoveConstantOverride(SR_UTILS_NS::StringAtom name) {
        SR_TRACY_ZONE;

        for (auto pIt = m_overrideConstant.begin(); pIt != m_overrideConstant.end(); ++pIt) {
            if (name == pIt->GetName()) {
                m_overrideConstant.erase(pIt);

                if (auto&& pPipeline = GetPipeline()) {
                    pPipeline->SetDirty(true);
                }

                return;
            }
        }
    }

    void Mesh::MarkUniformsDirty() {
        if (m_isUniformsDirty) SR_LIKELY_ATTRIBUTE {
            return;
        }

        SR_TRACY_ZONE;

        if (!IsMeshActive()) SR_UNLIKELY_ATTRIBUTE {
            return;
        }

        m_isUniformsDirty = !m_renderQueues.empty();

        auto pStart = m_renderQueues.data();
        auto pEnd = pStart + m_renderQueues.size();
        for (auto pElement = pStart; pElement != pEnd; ++pElement) {
            pElement->pRenderQueue->OnMeshDirty(this, pElement->shaderUseInfo);
        }
    }
}

