//
// Created by Monika on 05.10.2021.
//

#include <Graphics/Memory/MeshManager.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/Types/RawMesh.h>
#include <Utils/Common/StringAtomLiterals.h>
#include <Utils/Common/HashManager.h>
#include <Utils/Resources/ResourceManager.h>
#include <Utils/Debug.h>

namespace SR_GRAPH_NS {
    uint32_t MeshVideoMemoryInfo::Copy() {
        m_usages++;
        return m_memoryId;
    }

    void MeshVideoMemoryInfo::Use() {
        m_usages++;
    }

    void MeshVideoMemoryInfo::UnUse() {
        if (m_usages == 0) {
            SRHalt("MeshVideoMemoryInfo::UnUse() : no usages to unuse!");
        }
        m_usages--;
    }

    MeshManager::MeshManager() {
        m_indexBuffers.reserve(512);
        m_vertexBuffers.reserve(32);
    }

    MeshVideoMemoryInfo* MeshManager::Find(RegistrationInfo info, const SR_UTILS_NS::VertexLayoutDescription& layout) {
        SR_TRACY_ZONE;
        SR_LOCK_GUARD;

        if (info.isVBO) {
            auto&& pLayoutIt = std::ranges::find_if(m_vertexBuffers, [&layout](const VertexBufferInfo& info) {
                return info.layout.Compare(layout);
            });
            if (pLayoutIt != m_vertexBuffers.end()) {
                auto&& buffers = pLayoutIt->buffers;
                if (auto&& pInfo = buffers.Find(MeshVideoMemoryInfo(info))) {
                    return pInfo;
                }
            }
        }
        else {
            if (auto&& pInfo = m_indexBuffers.Find(MeshVideoMemoryInfo(info))) {
                return pInfo;
            }
        }

        return nullptr;
    }

    BakedMesh::~BakedMesh() {
        SRAssert2(m_usages == 0, "Not all uses were removed!");
        Destroy();
    }

    void BakedMesh::Destroy() {
        SR_TRACY_ZONE;

        if (m_pRawMesh) {
            m_pRawMesh->RemoveUsePoint();
            m_pRawMesh = nullptr;
        }

        if (m_VBO != SR_ID_INVALID) {
            if (MeshManager::Instance().Free(true, m_VBO) == MeshManager::FreeResult::Freed) {
                m_pipeline->FreeVBO(&m_VBO);
            }
            m_VBO = SR_ID_INVALID;
        }

        if (m_IBO != SR_ID_INVALID) {
            if (MeshManager::Instance().Free(false, m_IBO) == MeshManager::FreeResult::Freed) {
                m_pipeline->FreeIBO(&m_IBO);
            }
            m_IBO = SR_ID_INVALID;
        }
    }

    BakedMesh::Ptr BakedMesh::Bake(Pipeline* pPipeline, std::string_view path, uint32_t index, const SR_UTILS_NS::VertexLayoutDescription& layout) {
        if (auto&& pRawMesh = CoreResLoader::Load<SR_HTYPES_NS::RawMesh>(path)) {
            return Bake(pPipeline, pRawMesh.Get(), index, layout);
        }
        SR_ERROR("BakedMesh::Bake() : failed load raw mesh \"{}\"!", path);
        return nullptr;
    }

    BakedMesh::Ptr BakedMesh::Bake(Pipeline* pPipeline, SR_HTYPES_NS::RawMesh* pRawMesh, uint32_t index, const SR_UTILS_NS::VertexLayoutDescription& layout) {
        return MeshManager::Instance().BakeMesh(pPipeline, pRawMesh, index, layout);
    }

    BakedMesh::Ptr MeshManager::BakeMesh(Pipeline* pPipeline, SR_HTYPES_NS::RawMesh* pRawMesh, uint32_t index, const SR_UTILS_NS::VertexLayoutDescription& layout)  {
        SR_TRACY_ZONE;
        SR_LOCK_GUARD;

        if (!pRawMesh) {
            SR_ERROR("MeshManager::BakeMesh() : invalid raw mesh!");
            return nullptr;
        }

        if (!pPipeline) {
            SR_ERROR("MeshManager::BakeMesh() : invalid pipeline!");
            return nullptr;
        }

        if (index >= pRawMesh->GetMeshesCount()) {
            SR_ERROR("MeshManager::BakeMesh() : invalid index \"{}\"!", index);
            return nullptr;
        }

        RegistrationInfo registrationInfoVBO;
        registrationInfoVBO.resourceId = pRawMesh->GetResourceId();
        registrationInfoVBO.meshIndex = index;
        registrationInfoVBO.reloadCount = pRawMesh->GetReloadCount();
        registrationInfoVBO.isVBO = true;

        RegistrationInfo registrationInfoIBO = registrationInfoVBO;
        registrationInfoIBO.isVBO = false;

        BakedMesh::Ptr&& pBakedMesh = BakedMesh::MakeShared();

        if (auto&& VBO = CopyIfExists(registrationInfoVBO, layout); VBO != SR_ID_INVALID) {
            pBakedMesh->m_VBO = VBO;
        }

        if (auto&& IBO = CopyIfExists(registrationInfoIBO, layout); IBO != SR_ID_INVALID) {
            pBakedMesh->m_IBO = IBO;
        }

        if (pBakedMesh->m_VBO == SR_ID_INVALID) {
            const SR_UTILS_NS::VertexDataBuffer& buffer = pRawMesh->GetVertexBuffer(index, layout);
            pBakedMesh->m_VBO = pPipeline->AllocateVBO(buffer.GetDataSize(), buffer.GetRawData());
            if (pBakedMesh->m_VBO == SR_ID_INVALID) {
                SR_ERROR("MeshManager::BakeMesh() : failed allocate VBO for mesh \"{}\"!", pRawMesh->GetResourceId());
                return nullptr;
            }
            if (!Register(registrationInfoVBO, buffer.GetVertexCount(), pBakedMesh->m_VBO, layout)) {
                SR_ERROR("MeshManager::BakeMesh() : failed register VBO for mesh \"{}\"!", pRawMesh->GetResourceId());
                pPipeline->FreeVBO(&pBakedMesh->m_VBO);
                return nullptr;
            }
        }

        if (pBakedMesh->m_IBO == SR_ID_INVALID) {
            auto&& indices = pRawMesh->GetIndices(index);
            pBakedMesh->m_IBO = pPipeline->AllocateIBO(indices.data(), sizeof(uint32_t), indices.size(), pBakedMesh->m_VBO);
            bool hasError = pBakedMesh->m_IBO == SR_ID_INVALID;
            if (hasError) {
                SR_ERROR("MeshManager::BakeMesh() : failed allocate IBO for mesh \"{}\"!", pRawMesh->GetResourceId());
            }
            if (!hasError) {
                if (!Register(registrationInfoIBO, indices.size(), pBakedMesh->m_IBO, layout)) {
                    SR_ERROR("MeshManager::BakeMesh() : failed register IBO for mesh \"{}\"!", pRawMesh->GetResourceId());
                    hasError = true;
                }
            }
            if (hasError) {
                if (Free(true, pBakedMesh->m_VBO) == FreeResult::Freed) {
                    pPipeline->FreeVBO(&pBakedMesh->m_VBO);
                }
                return nullptr;
            }
        }

        pRawMesh->AddUsePoint();

        pBakedMesh->m_pRawMesh = pRawMesh;
        pBakedMesh->m_index = index;
        pBakedMesh->m_pipeline = pPipeline;
        pBakedMesh->m_countIndices = pRawMesh->GetIndicesCount(index);
        pBakedMesh->m_countVertices = pRawMesh->GetVerticesCount(index);

        return pBakedMesh;
    }

    bool MeshManager::Register(RegistrationInfo info, uint32_t size, uint32_t memoryId, const SR_UTILS_NS::VertexLayoutDescription& layout) {
        SR_TRACY_ZONE;
        SR_LOCK_GUARD;

        if (info.meshIndex == SR_ID_INVALID) {
            SRHalt("MeshManager::Register() : invalid mesh index!");
            return false;
        }

    #ifndef SR_RELEASE
        [[maybe_unused]] std::string_view debugResourceId = info.resourceId;
        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            SR_LOG("MeshManager::Register() : register resource \"{}\"...", info.resourceId);
        }
    #endif

        m_registration.resize(SR_MAX(m_registration.size(), memoryId + 1));
        MeshVideoMemoryInfo videoMemoryInfo(info, size, memoryId);
        videoMemoryInfo.Use();

        if (info.isVBO) {
            if (m_registration[memoryId].vertexBuffer) {
                SRHalt("MeshManager::Register() : memory id is already registered for VBO!");
                return false;
            }

            auto&& pLayoutIt = std::ranges::find_if(m_vertexBuffers, [&layout](const VertexBufferInfo& info) {
                return info.layout.Compare(layout);
            });

            if (pLayoutIt != m_vertexBuffers.end()) {
                pLayoutIt->buffers.Add(videoMemoryInfo);
            }
            else {
                VertexBufferInfo& vertexBufferInfo = m_vertexBuffers.emplace_back();
                vertexBufferInfo.layout = layout;
                vertexBufferInfo.buffers.Add(videoMemoryInfo);
            }

            m_registration[memoryId].vertexBuffer = info;
            m_registration[memoryId].vertexLayout = layout;
        }
        else {
            if (m_registration[memoryId].indexBuffer) {
                SRHalt("MeshManager::Register() : memory id is already registered for IBO!");
                return false;
            }
            m_indexBuffers.Add(videoMemoryInfo);
            m_registration[memoryId].indexBuffer = info;
        }

        return true;
    }

    MeshManager::FreeResult MeshManager::Free(bool isVBO, int32_t memoryId) {
        SR_TRACY_ZONE;
        SR_LOCK_GUARD;

        if (m_registration.size() <= memoryId) {
            SRHalt("MeshManager::Free() : invalid memory id!");
            return FreeResult::Invalid;
        }

        Registration& registration = m_registration[memoryId];
        if ((isVBO && !registration.vertexBuffer) || (!isVBO && !registration.indexBuffer)) {
            SRHalt("MeshManager::Free() : memory id isn't registered for this type!");
            return FreeResult::Invalid;
        }

        MeshVideoMemoryInfo* pMemoryInfo = isVBO ?
            Find(registration.vertexBuffer.value(), registration.vertexLayout) :
            Find(registration.indexBuffer.value(), SR_UTILS_NS::VertexLayoutDescription());

        if (!pMemoryInfo) {
            SRHalt("MeshManager::Free() : memory info not found!");
            return FreeResult::NotFound;
        }

        if (pMemoryInfo->GetUsages() > 1) {
            pMemoryInfo->UnUse();
            return FreeResult::EndUse;
        }

        if (isVBO) {
            auto&& pLayoutIt = std::ranges::find_if(m_vertexBuffers, [&registration](const VertexBufferInfo& info) {
                return info.layout.Compare(registration.vertexLayout);
            });

            if (SRVerify(pLayoutIt != m_vertexBuffers.end())) {
                auto&& buffers = pLayoutIt->buffers;
                if (!buffers.Remove(*pMemoryInfo)) {
                    SRHalt("MeshManager::Free() : failed remove VBO from buffers!");
                    return FreeResult::Invalid;
                }
                if (buffers.empty()) {
                    m_vertexBuffers.erase(pLayoutIt);
                }
                m_registration[memoryId].vertexBuffer.reset();
                m_registration[memoryId].vertexLayout = SR_UTILS_NS::VertexLayoutDescription();
            }
            return FreeResult::Freed;
        }

        if (!m_indexBuffers.Remove(*pMemoryInfo)) {
            SRHalt("MeshManager::Free() : failed remove IBO from buffers!");
            return FreeResult::Invalid;
        }
        m_registration[memoryId].indexBuffer.reset();
        return FreeResult::Freed;
    }

    int32_t MeshManager::CopyIfExists(RegistrationInfo info, const SR_UTILS_NS::VertexLayoutDescription& layout) {
        SR_LOCK_GUARD;
        SR_TRACY_ZONE;

    #ifndef SR_RELEASE
        [[maybe_unused]] std::string_view debugResourceId = info.resourceId;
    #endif

        if (auto&& pMemoryInfo = Find(info, layout)) {
            return pMemoryInfo->Copy();
        }
        return SR_ID_INVALID;
    }

    uint32_t MeshManager::Size(RegistrationInfo info, const SR_UTILS_NS::VertexLayoutDescription& layout) {
        SR_TRACY_ZONE;
        SR_LOCK_GUARD;
        if (auto&& pMemoryInfo = Find(info, layout)) {
            return pMemoryInfo->Size();
        }
        SRHalt("MeshManager::Size() : memory info not found!");
        return 0;
    }

    void MeshManager::OnSingletonDestroy() {
        if (!m_vertexBuffers.empty()) {
            SRHalt("MeshManager::OnSingletonDestroy() : not all vertex buffers were freed! Count: {}", m_vertexBuffers.size());
        }

        if (!m_indexBuffers.empty()) {
            SRHalt("MeshManager::OnSingletonDestroy() : not all index buffers were freed! Count: {}", m_indexBuffers.size());
        }
    }

    int32_t MeshManager::CopyIfExists(MeshManager::RegistrationInfo info) {
        if (info.isVBO) {
            SRHalt("MeshManager::CopyIfExists() : vertex buffer layout is required for VBO!");
            return SR_ID_INVALID;
        }
        return CopyIfExists(info, SR_UTILS_NS::VertexLayoutDescription());
    }

    bool MeshManager::Register(MeshManager::RegistrationInfo info, uint32_t size, uint32_t memoryId) {
        if (info.isVBO) {
            SRHalt("MeshManager::Register() : vertex buffer layout is required for VBO!");
            return false;
        }
        return Register(info, size, memoryId, SR_UTILS_NS::VertexLayoutDescription());
    }

    uint32_t MeshManager::Size(MeshManager::RegistrationInfo info) {
        if (info.isVBO) {
            SRHalt("MeshManager::Size() : vertex buffer layout is required for VBO!");
            return 0;
        }
        return Size(info, SR_UTILS_NS::VertexLayoutDescription());
    }
}
