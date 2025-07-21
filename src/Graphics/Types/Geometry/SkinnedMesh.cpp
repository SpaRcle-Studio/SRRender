//
// Created by Igor on 27/11/2022.
//

#include <Graphics/Types/Geometry/SkinnedMesh.h>

#include <Codegen/SkinnedMesh.generated.hpp>

namespace SR_GTYPES_NS {
    SkinnedMesh::SkinnedMesh()
        : Super()
    { }

    bool SkinnedMesh::Calculate()  {
        SR_TRACY_ZONE;

        if (IsCalculated()) {
            return true;
        }

        FreeVideoMemory();

        if (!IsCalculatable()) {
            return false;
        }

        if (!CalculateVBO<Vertices::VertexType::SkinnedMeshVertex, Vertices::SkinnedMeshVertex>([this]() {
            return Vertices::CastVertices<Vertices::SkinnedMeshVertex>(GetVertices());
        })) {
            return false;
        }

        const uint32_t sizeBones = GetRawMesh()->GetOptimizedBones().size() * sizeof(SR_MATH_NS::Matrix4x4);
        //const uint32_t sizeOffsets = GetRawMesh()->GetBoneOffsets().size() * sizeof(SR_MATH_NS::Matrix4x4);

        m_ssboBones = GetPipeline()->AllocateSSBO(sizeBones, SSBOUsage::CPUToGPU);
        //m_ssboOffsets = GetPipeline()->AllocateSSBO(sizeOffsets, SSBOUsage::CPUToGPU);

        return IndexedMesh::Calculate();
    }

    void SkinnedMesh::FreeSSBO() {
        if (m_ssboBones != SR_ID_INVALID) {
            GetPipeline()->FreeSSBO(&m_ssboBones);
        }

        //if (m_ssboOffsets != SR_ID_INVALID) {
        //    GetPipeline()->FreeSSBO(&m_ssboOffsets);
        //}
    }

    const SR_HTYPES_NS::FastMemoryArray<uint32_t>& SkinnedMesh::GetIndices() const {
        return GetRawMesh()->GetIndices(GetMeshId());
    }

    bool SkinnedMesh::IsCalculatable() const {
        return IsValidMeshId() && Mesh::IsCalculatable();
    }

    void SkinnedMesh::LateUpdate() {
        SR_TRACY_ZONE;

        auto&& pSkeleton = m_skeleton.Get();

        if (m_skeletonIsBroken && !pSkeleton) {
            return Super::LateUpdate();
        }

        if (!m_skeletonIsBroken && pSkeleton) {
            if (m_ssboBones == SR_ID_INVALID) { ///  || m_ssboOffsets == SR_ID_INVALID
                return Super::LateUpdate();
            }

            if (auto&& matrices = pSkeleton->GetMatrices(); !matrices.empty()) {
                GetPipeline()->UpdateSSBO(m_ssboBones, (void*)matrices.data(), matrices.size() * sizeof(SR_MATH_NS::Matrix4x4));
            }

            /// TODO: не обновлять каждый кадр, шарить между мешами
            //if (auto&& offsets = pSkeleton->GetOffsets(); !offsets.empty()) {
            //    GetPipeline()->UpdateSSBO(m_ssboOffsets, (void*)offsets.data(), offsets.size() * sizeof(SR_MATH_NS::Matrix4x4));
            //}

            return Super::LateUpdate();
        }

        m_skeletonIsBroken = !pSkeleton;
        m_renderScene->SetDirty();

        return Super::LateUpdate();
    };

    void SkinnedMesh::UseMaterial() {
        Super::UseMaterial();
        UseModelMatrix();
    }

    void SkinnedMesh::UseModelMatrix() {
        if (auto&& pShader = GetRenderContext()->GetCurrentShader()) {
            pShader->SetMat4(SHADER_MODEL_MATRIX, GetMatrix());
        }
    }

    bool SkinnedMesh::OnResourceReloaded(const SR_UTILS_NS::IResource* pResource) {
        bool changed = Mesh::OnResourceReloaded(pResource);
        if (GetRawMesh().Get() == pResource) {
            OnRawMeshChanged();
            return true;
        }
        return changed;
    }

    void SkinnedMesh::OnRawMeshChanged() {
        IRawMeshHolder::OnRawMeshChanged();

        ReRegisterMesh();

        MarkMaterialDirty();
        m_isCalculated = false;
    }

    std::string SkinnedMesh::GetMeshIdentifier() const {
        if (auto&& pRawMesh = GetRawMesh()) {
            return SR_FORMAT("{}|{}|{}", pRawMesh->GetResourceId().c_str(), GetMeshId(), pRawMesh->GetReloadCount());
        }

        return Super::GetMeshIdentifier();
    }

    void SkinnedMesh::FreeVideoMemory() {
        FreeSSBO();
        Super::FreeVideoMemory();
    }

    void SkinnedMesh::UseSSBO() {
        SR_TRACY_ZONE;

        //GetPipeline()->GetCurrentShader()->BindSSBO("offsets", m_ssboOffsets);
        GetPipeline()->GetCurrentShader()->BindSSBO("bones", m_ssboBones);

        auto&& pSkeleton = m_skeleton.Get();
        const int32_t offsetsSSBO = pSkeleton ? pSkeleton->GetOffsetsSSBO() : SR_ID_INVALID;

        if (offsetsSSBO != SR_ID_INVALID) {
            GetPipeline()->GetCurrentShader()->BindSSBO("offsets", offsetsSSBO);
        }
        else {
            GetPipeline()->GetCurrentShader()->BindSSBO("offsets", m_ssboBones);
        }

        Super::UseSSBO();
    }
}
