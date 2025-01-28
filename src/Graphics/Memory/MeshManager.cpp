//
// Created by Monika on 05.10.2021.
//

#include <Graphics/Memory/MeshManager.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/Types/RawMesh.h>
#include <Utils/Common/StringAtomLiterals.h>

namespace SR_GRAPH_NS::Memory {
    MeshManager::MeshManager() {
        const uint16_t reserve = 512;

        m_VBOs.reserve(reserve);
        m_IBOs.reserve(reserve);
        m_VBOTable.resize(reserve);
        m_IBOTable.resize(reserve);
    }

    MeshManager::VideoResourcesIter MeshManager::FindImpl(Hash hash, MeshMemoryType memType) {
        switch (memType) {
            case MeshMemoryType::VBO: {
                if (auto mem = m_VBOs.find(hash); mem != m_VBOs.end()) {
                    return mem;
                }
                break;
            }
            case MeshMemoryType::IBO: {
                if (auto mem = m_IBOs.find(hash); mem != m_IBOs.end()) {
                    return mem;
                }
                break;
            }
            default:
                SRHalt("MeshManager::FindImpl() : unknown memory type!");
                return std::nullopt;
        }

        return std::nullopt;
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
            if (MeshManager::Instance().Free<MeshMemoryType::VBO>(m_VBO) == MeshManager::FreeResult::Freed) {
                m_pipeline->FreeVBO(&m_VBO);
            }
            m_VBO = SR_ID_INVALID;
        }

        if (m_IBO != SR_ID_INVALID) {
            if (MeshManager::Instance().Free<MeshMemoryType::IBO>(m_IBO) == MeshManager::FreeResult::Freed) {
                m_pipeline->FreeIBO(&m_IBO);
            }
            m_IBO = SR_ID_INVALID;
        }
    }

    BakedMesh::Ptr BakedMesh::Bake(Pipeline *pPipeline, std::string_view path, uint32_t index, Vertices::VertexType vertexType) {
        if (auto&& pRawMesh = SR_HTYPES_NS::RawMesh::Load(path); pRawMesh) {
            return Bake(pPipeline, pRawMesh, index, vertexType);
        }
        SR_ERROR("BakedMesh::Bake() : failed load raw mesh \"{}\"!", path);
        return nullptr;
    }

    BakedMesh::Ptr BakedMesh::Bake(Pipeline *pPipeline, SR_HTYPES_NS::RawMesh *pRawMesh, uint32_t index, Vertices::VertexType vertexType) {
        return MeshManager::Instance().BakeMesh(pPipeline, pRawMesh, index, vertexType);
    }

    BakedMesh::Ptr MeshManager::BakeMesh(Pipeline* pPipeline, SR_HTYPES_NS::RawMesh* pRawMesh, uint32_t index, Vertices::VertexType vertexType)  {
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

        const std::string id = "{}|{}|{}"_format(pRawMesh->GetResourceId().c_str(), index, pRawMesh->GetReloadCount());

        BakedMesh::Ptr&& pBakedMesh = BakedMesh::MakeShared();

        if (auto&& VBO = CopyIfExists<MeshMemoryType::VBO>(id, vertexType); VBO != SR_ID_INVALID) {
            pBakedMesh->m_VBO = VBO;
        }

        if (auto&& IBO = CopyIfExists<MeshMemoryType::IBO>(id, Vertices::VertexType::Unknown); IBO != SR_ID_INVALID) {
            pBakedMesh->m_IBO = IBO;
        }

        if (pBakedMesh->m_VBO == SR_ID_INVALID) {
            auto&& vertices = pRawMesh->GetVertices(index);
            pBakedMesh->m_VBO = pPipeline->AllocateVBO(vertices.data(), vertexType, vertices.size());
            if (pBakedMesh->m_VBO == SR_ID_INVALID) {
                SR_ERROR("MeshManager::BakeMesh() : failed allocate VBO for mesh \"{}\"!", id);
                return nullptr;
            }
            if (!Register<MeshMemoryType::VBO>(id, vertices.size(), pBakedMesh->m_VBO, vertexType)) {
                SR_ERROR("MeshManager::BakeMesh() : failed register VBO for mesh \"{}\"!", id);
                pPipeline->FreeVBO(&pBakedMesh->m_VBO);
                return nullptr;
            }
        }

        if (pBakedMesh->m_IBO == SR_ID_INVALID) {
            auto&& indices = pRawMesh->GetIndices(index);
            pBakedMesh->m_IBO = pPipeline->AllocateIBO(indices.data(), sizeof(uint32_t), indices.size(), pBakedMesh->m_VBO);
            bool hasError = pBakedMesh->m_IBO == SR_ID_INVALID;
            if (hasError) {
                SR_ERROR("MeshManager::BakeMesh() : failed allocate IBO for mesh \"{}\"!", id);
            }
            if (!hasError) {
                hasError |= !Register<MeshMemoryType::IBO>(id, indices.size(), pBakedMesh->m_IBO, Vertices::VertexType::Unknown);
            }
            if (hasError) {
                if (Free<MeshMemoryType::VBO>(pBakedMesh->m_VBO) == FreeResult::Freed) {
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

    bool MeshManager::RegisterImpl(const std::string_view& resourceId, MeshMemoryType memType, uint32_t size, uint32_t id) {
    #ifndef SR_RELEASE
        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            SR_LOG("MeshManager::RegisterImpl() : register resource \"{}\"...", resourceId);
        }
    #endif

        SRAssert2(id <= 32768, "Buffer overflow!");

        Hash hash = SR_HASH_STR_VIEW(resourceId);

        switch (memType) {
            case MeshMemoryType::VBO: {
                m_VBOs[hash] = MeshVidMemInfo(size, id, memType);

                if (id >= m_VBOTable.size()) {
                    m_VBOTable.resize(SR_MAX(m_VBOTable.size() * 2, id + 1));
                }
                m_VBOTable[id] = hash;

                return true;
            }
            case MeshMemoryType::IBO: {
                m_IBOs[hash] = MeshVidMemInfo(size, id, memType);

                if (id >= m_IBOTable.size()) {
                    m_IBOTable.resize(SR_MAX(m_IBOTable.size() * 2, id + 1));
                }
                m_IBOTable[id] = hash;

                return true;
            }

            default:
                SR_ERROR("MeshManager::RegisterImpl() : unknown type!");
                return false;
        }
    }

    MeshManager::FreeResult MeshManager::FreeImpl(VideoResourcesIter iter, MeshMemoryType memType) {
        if (auto& memory = iter.value()->second; memory.m_usages == 1) {
            switch (memType) {
                case MeshMemoryType::VBO: m_VBOs.erase(iter.value());
                    goto skip;
                case MeshMemoryType::IBO: m_IBOs.erase(iter.value());
                skip:
                    return FreeResult::Freed;
                case MeshMemoryType::Unknown:
                default:
                    SRHalt("MeshManager::FreeImpl() : unknown memory type!");
                    return FreeResult::UnknownMem;
            }
        }
        else {
            --memory.m_usages;
            return FreeResult::EndUse;
        }
    }

    void MeshManager::OnSingletonDestroy() {
        if (!m_VBOs.empty()) {
            SR_WARN("MeshManager::OnSingletonDestroy() : VBOs isn't empty! \n\tCount = {} \n\tMemory leak possible.", m_VBOs.size());
        }

        if (!m_IBOs.empty()) {
            SR_WARN("MeshManager::OnSingletonDestroy() : IBOs isn't empty! \n\tCount = {} \n\tMemory leak possible.", m_IBOs.size());
        }

        m_VBOs.clear();
        m_IBOs.clear();

        m_VBOTable.clear();
        m_IBOTable.clear();
    }

    MeshManager::VideoResourcesIter MeshManager::FindById(int32_t id, MeshMemoryType memType) {
        HashTable* pHashTable = nullptr;

        switch (memType) {
            case MeshMemoryType::VBO: pHashTable = &m_VBOTable; break;
            case MeshMemoryType::IBO: pHashTable = &m_IBOTable; break;
            case MeshMemoryType::Unknown:
            default:
                SRHalt("MeshManager::FindById() : unknown memory type!");
                return std::nullopt;
        }

        if (id >= pHashTable->size()) {
            SRHalt("MeshManager::FindById() : invalid id!");
            return std::nullopt;
        }

        return FindImpl((*pHashTable)[id], memType);
    }

    uint32_t MeshVidMemInfo::Copy() {
    #ifndef SR_RELEASE
        if (m_type == MeshMemoryType::Unknown) {
            SR_WARN("MeshVidMemInfo::Copy() : unknown memory type!");
        }

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            switch (m_type) {
                case MeshMemoryType::VBO: SR_LOG("MeshVidMemInfo::Copy() : copy VBO..."); break;
                case MeshMemoryType::IBO: SR_LOG("MeshVidMemInfo::Copy() : copy IBO..."); break;
                default: break;
            }
        }
    #endif

        m_usages++;
        return m_vidId;
    }
}
