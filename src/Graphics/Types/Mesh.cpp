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
        SRAssert(m_virtualUBO == SR_ID_INVALID);
        SRAssert(m_virtualDescriptor == SR_ID_INVALID);
    }

    Mesh::Ptr Mesh::Load(const SR_UTILS_NS::Path& path, uint32_t id) {
        if (auto&& pRawMesh =  CoreResLoader::Load<SR_HTYPES_NS::RawMesh>(path)) {
            return TryLoad(pRawMesh.Get(), id);
        }
        SR_ERROR("Mesh::Load() : failed to load mesh!\n\tPath: {}\n\tId: {}", path, id);
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
        return Super::IsActive() && !m_hasErrors;
    }

    void Mesh::FreeVideoMemory() {
        if (auto&& pPipeline = TryGetPipeline()) {
            pPipeline->GetUBOManager().TryFreeUBO(&m_virtualUBO);
            pPipeline->GetDescriptorManager().TryFreeDescriptorSet(&m_virtualDescriptor);
        }
        Super::FreeVideoMemory();
    }

    bool Mesh::Calculate() {
        m_isCalculated = true;
        /// чтобы в случае перезагрузки обновить все связанные данные
        MarkMaterialDirty();
        return true;
    }

    bool Mesh::Bind() {
        SR_TRACY_ZONE;

        if (auto&& VBO = GetVBO(); VBO && VBO != SR_ID_INVALID) SR_LIKELY_ATTRIBUTE {
            GetPipeline()->BindVBO(VBO.value(), VertexInputRate::Vertex);
        }
        else {
            return false;
        }

        if (auto&& IBO = GetIBO(); IBO && IBO != SR_ID_INVALID) SR_LIKELY_ATTRIBUTE {
            GetPipeline()->BindIBO(IBO.value());
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

        DrawRenderObject(this, GetIndicesCount(), m_virtualUBO, m_virtualDescriptor, m_dirtyMaterial, m_hasErrors);
    }

    bool Mesh::OnResourceReloaded(SR_UTILS_NS::StringAtom resourceId) {
        return false;
    }

    Mesh::Ptr Mesh::Load(const SR_UTILS_NS::Path& path, SR_UTILS_NS::StringAtom name) {
        if (auto&& pRawMesh = CoreResLoader::Load<SR_HTYPES_NS::RawMesh>(path)) {
            return Load(path, pRawMesh->GetMeshId(name));
        }
        return nullptr;
    }

    void Mesh::SetVertexLayoutDescription(const SR_UTILS_NS::VertexLayoutDescription& description) {
        Super::SetVertexLayoutDescription(description);
        m_isCalculated = false;
    }
}

