//
// Created by Igor on 27/11/2022.
//

#include <Graphics/Types/Geometry/SkinnedMesh.h>

namespace SR_GTYPES_NS {
    SkinnedMesh::SkinnedMesh()
        : Super(MeshType::Skinned)
    {
        m_entityMessages.AddCustomProperty<SR_UTILS_NS::LabelProperty>("SkeletonInv")
            .SetLabel("Skeleton is not usable!")
            .SetColor(SR_MATH_NS::FColor(1.f, 0.f, 0.f, 1.f))
            .SetActiveCondition([this] { return !IsSkeletonUsable(); })
            .SetDontSave();
    }

    SkinnedMesh::~SkinnedMesh() {
        SetRawMesh(nullptr);
    }

    bool SkinnedMesh::Calculate()  {
        SR_TRACY_ZONE;

        if (IsCalculated()) {
            return true;
        }

        FreeVideoMemory();

        if (!IsCalculatable()) {
            return false;
        }

        if (SR_UTILS_NS::Debug::Instance().GetLevel() >= SR_UTILS_NS::Debug::Level::High) {
            SR_LOG("SkinnedMesh::Calculate() : calculating \"" + m_geometryName + "\"...");
        }

        if (!CalculateVBO<Vertices::VertexType::SkinnedMeshVertex, Vertices::SkinnedMeshVertex>([this]() {
            return Vertices::CastVertices<Vertices::SkinnedMeshVertex>(GetVertices());
        })) {
            return false;
        }

        const uint32_t sizeBones = GetRawMesh()->GetOptimizedBones().size() * sizeof(SR_MATH_NS::Matrix4x4);
        const uint32_t sizeOffsets = GetRawMesh()->GetBoneOffsets().size() * sizeof(SR_MATH_NS::Matrix4x4);

        m_ssboBones = GetPipeline()->AllocateSSBO(sizeBones, SSBOUsage::Write);
        m_ssboOffsets = GetPipeline()->AllocateSSBO(sizeOffsets, SSBOUsage::Write);

        return IndexedMesh::Calculate();
    }

    void SkinnedMesh::FreeSSBO() {
        if (m_ssboBones != SR_ID_INVALID) {
            GetPipeline()->FreeSSBO(&m_ssboBones);
        }

        if (m_ssboOffsets != SR_ID_INVALID) {
            GetPipeline()->FreeSSBO(&m_ssboOffsets);
        }
    }

    std::vector<uint32_t> SkinnedMesh::GetIndices() const {
        return GetRawMesh()->GetIndices(GetMeshId());
    }

    bool SkinnedMesh::IsCalculatable() const {
        return IsValidMeshId() && Mesh::IsCalculatable();
    }

    bool SkinnedMesh::IsSkeletonUsable() const {
        return GetSkeleton().GetComponent<SR_ANIMATIONS_NS::Skeleton>();
    }

    void SkinnedMesh::LateUpdate() {
        SR_TRACY_ZONE;

        const bool usable = IsSkeletonUsable();

        if (m_skeletonIsBroken && !usable) {
            return Super::LateUpdate();
        }

        if (!m_skeletonIsBroken && usable) {
            if (m_ssboBones == SR_ID_INVALID || m_ssboOffsets == SR_ID_INVALID) {
                return Super::LateUpdate();
            }
            auto&& pSkeleton = GetSkeleton().GetComponent<SR_ANIMATIONS_NS::Skeleton>();
            if (!pSkeleton || pSkeleton->GetOptimizedBones().empty()) {
                return Super::LateUpdate();
            }
            GetPipeline()->UpdateSSBO(m_ssboBones, (void*)pSkeleton->GetMatrices().data(), pSkeleton->GetMatrices().size() * sizeof(SR_MATH_NS::Matrix4x4));
            GetPipeline()->UpdateSSBO(m_ssboOffsets, (void*)pSkeleton->GetOffsets().data(), pSkeleton->GetOffsets().size() * sizeof(SR_MATH_NS::Matrix4x4));
            return Super::LateUpdate();
        }

        m_skeletonIsBroken = !usable;
        m_renderScene->SetDirty();

        return Super::LateUpdate();
    };

    void SkinnedMesh::UseMaterial() {
        Super::UseMaterial();
        UseModelMatrix();
    }

    void SkinnedMesh::UseModelMatrix() {
        SR_TRACY_ZONE;
        /// TODO: А не стоило бы изменить ColorBufferPass так, чтобы он вызывал не UseModelMatrix, а более обощённый метод?
        /// Нет, не стоило бы.
        if (!PopulateSkeletonMatrices()) {
            return;
        }

        auto&& pShader = GetRenderContext()->GetCurrentShader();
        SRAssert(pShader);

        pShader->SetMat4(SHADER_MODEL_MATRIX, GetMatrix());

        auto&& pSkeleton = GetSkeleton().GetComponent<SR_ANIMATIONS_NS::Skeleton>();
        auto&& pRenderScene = GetRenderScene();

        SRAssert(pRenderScene);

        //if (pRenderScene->GetCurrentSkeleton() == pSkeleton.Get()) {
        //    return;
        //}

        pRenderScene->SetCurrentSkeleton(pSkeleton.Get());


        //GetRawMesh()->GetOptimizedBones().size();
        //switch (GetMaxBones()) {
        //    case 128:
        //        pShader->SetValue<false>(SHADER_SKELETON_MATRICES_128, pSkeleton->GetMatrices().data());
        //        pShader->SetValue<false>(SHADER_SKELETON_MATRIX_OFFSETS_128, pSkeleton->GetOffsets().data());
        //        break;
        //    case 256:
        //        pShader->SetValue<false>(SHADER_SKELETON_MATRICES_256, pSkeleton->GetMatrices().data());
        //        pShader->SetValue<false>(SHADER_SKELETON_MATRIX_OFFSETS_256, pSkeleton->GetOffsets().data());
        //        break;
        //    case 384:
        //        pShader->SetValue<false>(SHADER_SKELETON_MATRICES_384, pSkeleton->GetMatrices().data());
        //        pShader->SetValue<false>(SHADER_SKELETON_MATRIX_OFFSETS_384, pSkeleton->GetOffsets().data());
        //        break;
        //    case 0:
        //        break;
        //    default:
        //        SRHaltOnce0();
        //        return;
        //}
    }

    bool SkinnedMesh::OnResourceReloaded(SR_UTILS_NS::IResource* pResource) {
        bool changed = Mesh::OnResourceReloaded(pResource);
        if (GetRawMesh() == pResource) {
            OnRawMeshChanged();
            return true;
        }
        return changed;
    }

    bool SkinnedMesh::PopulateSkeletonMatrices() {
        SR_TRACY_ZONE;

        auto&& bones = GetRawMesh()->GetOptimizedBones();

        if (bones.empty()) {
            return false;
        }

        auto&& pSkeleton = GetSkeleton().GetComponent<SR_ANIMATIONS_NS::Skeleton>();

        if (!pSkeleton || !pSkeleton->GetRootBone()) {
            return false;
        }

        pSkeleton->SetOptimizedBones(GetRawMesh()->GetOptimizedBones());
        pSkeleton->SetBonesOffsets(GetRawMesh()->GetBoneOffsets());

        return true;
    }

    void SkinnedMesh::OnRawMeshChanged() {
        IRawMeshHolder::OnRawMeshChanged();

        if (GetRawMesh() && IsValidMeshId()) {
            SetGeometryName(GetRawMesh()->GetGeometryName(GetMeshId()));
        }

        if (GetSkeleton().IsValid()) {
            if (auto&& pSkeleton = GetSkeleton().GetComponent<SR_ANIMATIONS_NS::Skeleton>()) {
                pSkeleton->ResetSkeleton();
            }
        }

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

    bool SkinnedMesh::InitializeEntity() noexcept {
        m_properties.AddCustomProperty<SR_UTILS_NS::PathProperty>("Mesh")
            .AddFileFilter("Mesh", SR_GRAPH_NS::SR_SUPPORTED_MESH_FORMATS)
            .SetGetter([this]()-> SR_UTILS_NS::Path {
                return GetRawMesh() ? GetRawMesh()->GetResourcePath() : SR_UTILS_NS::Path();
            })
            .SetSetter([this](const SR_UTILS_NS::Path& path) {
                SetRawMesh(path);
            });

        m_properties.AddCustomProperty<SR_UTILS_NS::StandardProperty>("Index")
            .SetGetter([this](void* pData) {
                *reinterpret_cast<int16_t*>(pData) = static_cast<int16_t>(GetMeshId());
            })
            .SetSetter([this](void* pData) {
                SetMeshId(static_cast<MeshIndex>(*reinterpret_cast<int16_t*>(pData)));
            })
            .SetType(SR_UTILS_NS::StandardType::Int16);

        m_properties.AddEntityRefProperty(SR_SKELETON_REF_PROP_NAME, GetThis())
            .SetWidth(260.f);

        return Super::InitializeEntity();
    }

    SR_UTILS_NS::EntityRef& SkinnedMesh::GetSkeleton() const {
        auto&& pSkeletonRefProperty = GetComponentProperties().Find<SR_UTILS_NS::EntityRefProperty>(SR_SKELETON_REF_PROP_NAME);
        if (pSkeletonRefProperty) {
            return pSkeletonRefProperty->GetEntityRef();
        }
        SRHaltOnce0();
        static SR_UTILS_NS::EntityRef defaultEntityRef;
        return defaultEntityRef;
    }

    void SkinnedMesh::FreeVideoMemory() {
        FreeSSBO();
        Super::FreeVideoMemory();
    }

    void SkinnedMesh::UseSSBO() {
        GetPipeline()->GetCurrentShader()->BindSSBO("bones", m_ssboBones);
        GetPipeline()->GetCurrentShader()->BindSSBO("offsets", m_ssboOffsets);
        Super::UseSSBO();
    }
}
