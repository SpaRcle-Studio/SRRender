//
// Created by Monika on 30.10.2021.
//

#include <Graphics/Types/Geometry/IndexedMesh.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Memory/MeshManager.h>

#include <Utils/Types/IRawMeshHolder.h>

#include <Codegen/IndexedMesh.generated.hpp>

namespace SR_GTYPES_NS {
    IndexedMesh::~IndexedMesh() {
        SRAssert(m_IBO == SR_ID_INVALID && m_VBO == SR_ID_INVALID);
    }

    bool IndexedMesh::Calculate() {
        SR_TRACY_ZONE;

        if (IsCalculated()) {
            return true;
        }

        FreeVideoMemory();

        if (!IsCalculatable()) {
            return false;
        }

        m_isUniqueMesh = true;
        if (dynamic_cast<SR_HTYPES_NS::IRawMeshHolder*>(this)) {
            m_isUniqueMesh = false;
        }

        if (!CalculateIBO() || !CalculateVBO()) {
            return false;
        }
        return Super::Calculate();
    }

    bool IndexedMesh::CalculateIBO() {
        SR_TRACY_ZONE;

        if (!SRVerify2(m_IBO == SR_ID_INVALID, "IBO already calculated!")) SR_UNLIKELY_ATTRIBUTE {
            return false;
        }

        auto&& indices = GetIndices();
        if ((m_countIndices = indices.size()) == 0) {
            SR_ERROR("IndexedMesh::CalculateIBO() : invalid indices!");
            return false;
        }

        if (m_isUniqueMesh) {
            if (m_IBO = GetPipeline()->AllocateIBO((void *)indices.data(), sizeof(uint32_t), m_countIndices, m_VBO); m_IBO == SR_ID_INVALID) {
                SR_ERROR("IndexedMesh::CalculateIBO() : failed calculate IBO for mesh!");
                m_hasErrors = true;
                return false;
            }
            return true;
        }

        MeshVideoMemoryInfo::RegistrationInfo registrationInfo;
        if (auto&& pRawMeshHolder = dynamic_cast<SR_HTYPES_NS::IRawMeshHolder*>(this); pRawMeshHolder && pRawMeshHolder->IsValidMeshId()) {
            registrationInfo.resourceId = pRawMeshHolder->GetRawMesh()->GetResourceId();
            registrationInfo.reloadCount = pRawMeshHolder->GetRawMesh()->GetReloadCount();
            registrationInfo.meshIndex = pRawMeshHolder->GetMeshId();
            registrationInfo.isVBO = false;
        }
        else {
            SR_ERROR("IndexedMesh::CalculateIBO() : failed get registration info for mesh!");
            return false;
        }

        m_IBO = MeshManager::Instance().CopyIfExists(registrationInfo);
        if (m_IBO == SR_ID_INVALID) {
            if (m_IBO = GetPipeline()->AllocateIBO((void *) indices.data(), sizeof(uint32_t), m_countIndices, m_VBO); m_IBO == SR_ID_INVALID) {
                SR_ERROR("IndexedMesh::CalculateIBO() : failed calculate IBO for mesh!");
                m_hasErrors = true;
                return false;
            }
            return MeshManager::Instance().Register(registrationInfo, m_countIndices, m_IBO);
        }
        m_countIndices = MeshManager::Instance().Size(registrationInfo);
        return true;
    }

    bool IndexedMesh::FreeIBO() {
        if (m_IBO != SR_ID_INVALID) {
            const bool isAllowFree = m_isUniqueMesh || MeshManager::Instance().Free(false, m_IBO) == MeshManager::FreeResult::Freed;
            if (isAllowFree && !GetPipeline()->FreeIBO(&m_IBO)) {
                SR_ERROR("IndexedMesh:FreeIBO() : failed free IBO! Something went wrong...");
                return false;
            }
            m_IBO = SR_ID_INVALID;
        }
        return true;
    }

    bool IndexedMesh::FreeVBO() {
        if (m_VBO != SR_ID_INVALID) {
            const bool isAllowFree = m_isUniqueMesh || MeshManager::Instance().Free(true, m_VBO) == MeshManager::FreeResult::Freed;
            if (isAllowFree && !GetPipeline()->FreeVBO(&m_VBO)) {
                SR_ERROR("IndexedMesh::FreeVBO() : failed free VBO! Something went wrong...");
                return false;
            }
            m_VBO = SR_ID_INVALID;
        }
        return true;
    }

    void IndexedMesh::FreeVideoMemory() {
        SR_TRACY_ZONE;

        Super::FreeVideoMemory();

        if (!FreeVBO()) {
            SR_ERROR("IndexedMesh::FreeVideoMemory() : failed to free VBO!");
        }

        if (!FreeIBO()) {
            SR_ERROR("IndexedMesh::FreeVideoMemory() : failed to free IBO!");
        }
    }

    bool IndexedMesh::CalculateVBO() {
        SR_TRACY_ZONE;

        SRAssert(m_VBO == SR_ID_INVALID);

        const SR_UTILS_NS::VertexLayoutDescription& vertexLayout = GetVertexLayoutDescription();
        const SR_UTILS_NS::VertexDataBuffer& buffer = GetVertices();
        if ((m_countVertices = buffer.GetVertexCount()) == 0) {
            SR_ERROR("IndexedMesh::CalculateVBO() : invalid vertices!");
            return false;
        }

        if (m_isUniqueMesh) {
            if (m_VBO = GetPipeline()->AllocateVBO(SR_INVALID_VBO, buffer.GetDataSize(), buffer.GetRawData()); m_VBO == SR_ID_INVALID) {
                SR_ERROR("IndexedMesh::CalculateVBO() : failed calculate VBO for mesh!");
                m_hasErrors = true;
                return false;
            }
            return true;
        }

        MeshVideoMemoryInfo::RegistrationInfo registrationInfo;
        registrationInfo.isVBO = true;
        if (auto&& pRawMeshHolder = dynamic_cast<SR_HTYPES_NS::IRawMeshHolder*>(this); pRawMeshHolder && pRawMeshHolder->IsValidMeshId()) {
            registrationInfo.resourceId = pRawMeshHolder->GetRawMesh()->GetResourceId();
            registrationInfo.reloadCount = pRawMeshHolder->GetRawMesh()->GetReloadCount();
            registrationInfo.meshIndex = pRawMeshHolder->GetMeshId();
        }

        m_VBO = MeshManager::Instance().CopyIfExists(registrationInfo, vertexLayout);
        if (m_VBO == SR_ID_INVALID) {
            if (m_VBO = GetPipeline()->AllocateVBO(SR_INVALID_VBO, buffer.GetDataSize(), buffer.GetRawData()); m_VBO == SR_ID_INVALID) {
                SR_ERROR("IndexedMesh::CalculateVBO() : failed calculate VBO for mesh!");
                m_hasErrors = true;
                return false;
            }
            return MeshManager::Instance().Register(registrationInfo, buffer.GetVertexCount(), m_VBO, vertexLayout);
        }
        m_countVertices = MeshManager::Instance().Size(registrationInfo, vertexLayout);
        return true;
    }

    std::optional<int32_t> IndexedMesh::GetVBO() const {
        if (!IsCalculated() && !const_cast<IndexedMesh*>(this)->Calculate()) SR_UNLIKELY_ATTRIBUTE {
            return SR_INVALID_VBO;
        }
        return m_VBO;
    }

    std::optional<int32_t> IndexedMesh::GetIBO() const {
        if (!IsCalculated() && !const_cast<IndexedMesh*>(this)->Calculate()) SR_UNLIKELY_ATTRIBUTE {
            return SR_INVALID_IBO;
        }
        return m_IBO;
    }

    const SR_UTILS_NS::VertexLayoutDescription& IndexedMesh::GetShaderVertexLayoutDescription() const noexcept {
        auto&& buffer = GetVertices();
        return buffer.layout.attributesCount > 0 ? buffer.layout : Super::GetShaderVertexLayoutDescription();
    }
}