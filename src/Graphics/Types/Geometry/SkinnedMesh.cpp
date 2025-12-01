//
// Created by Igor on 27/11/2022.
//

#include <Graphics/Types/Geometry/SkinnedMesh.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Render/RenderScene.h>

#include <Utils/Common/Features.h>
#include <Utils/FileSystem/PathDataAccessor.h>

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

        FreeVMemory();

        if (!IsCalculatable()) {
            return false;
        }

        if (!CalculateVBO<Vertices::VertexType::SkinnedMeshVertex, Vertices::SkinnedMeshVertex>([this]() {
            return Vertices::CastVertices<Vertices::SkinnedMeshVertex>(GetVertices());
        })) {
            return false;
        }

        const uint32_t sizeBones = GetRawMesh()->GetOptimizedBones().size() * sizeof(SR_MATH_NS::Matrix4x4);
        const uint8_t maxFrames = SR_UTILS_NS::Features::Instance().Enabled("MultiFrameSSBOResources", true) ? GetPipeline()->GetSwapchainImagesCount() : 1;

        m_ssboBones.resize(maxFrames, SR_ID_INVALID);
        for (uint8_t frame = 0; frame < maxFrames; ++frame) {
            m_ssboBones[frame] = GetPipeline()->AllocateSSBO(sizeBones, SSBOUsage::CPUToGPU);
        }

        return IndexedMesh::Calculate();
    }

    void SkinnedMesh::FreeSSBO() {
        for (auto& ssbo : m_ssboBones) {
            if (ssbo != SR_ID_INVALID) {
                GetPipeline()->FreeSSBO(&ssbo);
            }
        }
        m_ssboBones.clear();
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

        if (!m_skeletonIsBroken && pSkeleton && !m_ssboBones.empty()) {
            const uint8_t frame = GetPipeline()->GetCurrentFrameIndex();
            const auto ssbo = m_ssboBones[SR_MIN(frame, m_ssboBones.size() - 1)];
            if (ssbo == SR_ID_INVALID) {
                return Super::LateUpdate();
            }

            if (auto&& matrices = pSkeleton->GetMatrices(); !matrices.empty()) {
                GetPipeline()->UpdateSSBO(ssbo, (void*)matrices.data(), matrices.size() * sizeof(SR_MATH_NS::Matrix4x4));
            }

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
        if (auto&& pShader = GetPipeline()->GetCurrentShader()) {
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

    void SkinnedMesh::FreeVMemory() {
        FreeSSBO();
        Super::FreeVMemory();
    }

    void SkinnedMesh::UseSSBO() {
        SR_TRACY_ZONE;

        if (m_ssboBones.empty()) {
            return Super::UseSSBO();
        }

        const uint8_t frame = GetPipeline()->GetCurrentFrameIndex();
        const auto ssbo = m_ssboBones[SR_MIN(frame, m_ssboBones.size() - 1)];

        GetPipeline()->GetCurrentShader()->BindSSBO("bones", ssbo);

        auto&& pSkeleton = m_skeleton.Get();
        const int32_t offsetsSSBO = pSkeleton ? pSkeleton->GetOffsetsSSBO() : SR_ID_INVALID;

        if (offsetsSSBO != SR_ID_INVALID) {
            GetPipeline()->GetCurrentShader()->BindSSBO("offsets", offsetsSSBO);
        }
        else {
            GetPipeline()->GetCurrentShader()->BindSSBO("offsets", ssbo);
        }

        Super::UseSSBO();
    }
}
