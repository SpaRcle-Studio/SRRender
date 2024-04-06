//
// Created by Nikita on 17.11.2020.
//

#include <Utils/ECS/Component.h>
#include <Utils/Resources/ResourceManager.h>
#include <Utils/Resources/IResource.h>

#include <Graphics/Types/Mesh.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Utils/MeshUtils.h>

namespace SR_GRAPH_NS::Types {
    Mesh::Mesh(MeshType type)
        : m_uboManager(Memory::UBOManager::Instance())
        , m_meshType(type)
        , m_material(nullptr)
    { }

    Mesh::~Mesh() {
        SetMaterial(nullptr);
        SRAssert(m_virtualUBO == SR_ID_INVALID);
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
            pRawMesh->CheckResourceUsage();
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

        IGraphicsResource::FreeVideoMemory();
    }

    bool Mesh::Calculate() {
        m_isCalculated = true;
        /// чтобы в случае перезагрузки обновить все связанные данные
        m_dirtyMaterial = true;
        return true;
    }

    void Mesh::SetMaterial(const SR_UTILS_NS::Path& path) {
        SR_TRACY_ZONE;
        if (m_material && m_material->GetResourcePath() == path) {
            return;
        }
        SetMaterial(path.empty() ? nullptr : Material::Load(path));
    }

    void Mesh::SetMaterial(MaterialPtr pMaterial) {
        SR_TRACY_ZONE;

        if (pMaterial == m_material) {
            return;
        }

        m_dirtyMaterial = true;
        m_hasErrors = false;

        if (m_material) {
            m_material->RemoveUsePoint();
        }

        if ((m_material = pMaterial)) {
            m_material->AddUsePoint();
        }

        ReRegisterMesh();
    }

    Mesh::ShaderPtr Mesh::GetShader() const {
        return m_material ? m_material->GetShader() : nullptr;
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

        m_material->Use();
    }

    bool Mesh::BindMesh() {
        if (auto&& VBO = GetVBO(); VBO != SR_ID_INVALID) {
            m_pipeline->BindVBO(VBO);
        }
        else {
            return false;
        }

        if (auto&& IBO = GetIBO(); IBO != SR_ID_INVALID) {
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
        if (!m_material) {
            return;
        }

        m_material->UseSamplers();

        if (auto&& pShader = m_pipeline->GetCurrentShader()) {
            pShader->FlushSamplers();
        }
    }

    std::string Mesh::GetMeshIdentifier() const {
        static const std::string empty;
        return empty;
    }

    bool Mesh::OnResourceReloaded(SR_UTILS_NS::IResource* pResource) {
        if (m_material) {
            if (pResource == m_material) {
                m_dirtyMaterial = true;
                m_hasErrors = false;
                ReRegisterMesh();
                return true;
            }

            if (m_material->GetShader() == pResource) {
                m_dirtyMaterial = true;
                m_hasErrors = false;
                ReRegisterMesh();
                return true;
            }

            auto&& pTexture = dynamic_cast<SR_GTYPES_NS::Texture*>(pResource);
            if (pTexture && m_material->ContainsTexture(pTexture)) {
                m_dirtyMaterial = true;
                m_hasErrors = false;
                return true;
            }
        }
        return false;
    }

    void Mesh::MarkMaterialDirty() {
        m_dirtyMaterial = true;
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

    bool Mesh::UnRegisterMesh() {
        const bool isRegistered = IsMeshRegistered();

        if (isRegistered) {
            m_registrationInfo.value().pScene->Remove(this);

            if (IsCalculated()) {
                FreeVideoMemory();
                DeInitGraphicsResource();
            }
        }
        else {
            SRAssert(!IsCalculated());
        }

        if (auto&& pRenderComponent = dynamic_cast<IRenderComponent*>(this)) {
            pRenderComponent->AutoFree();
        }
        else {
            delete this;
        }

        return isRegistered;
    }

    void Mesh::ReRegisterMesh() {
        SR_TRACY_ZONE;
        if (m_registrationInfo.has_value()) {
            auto pRenderScene = m_registrationInfo.value().pScene;
            pRenderScene->ReRegister(m_registrationInfo.value());
        }
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
}

