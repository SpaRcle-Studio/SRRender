//
// Created by Monika on 11.01.2023.
//

#include <Graphics/Animations/Skeleton.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Utils/MeshUtils.h>
#include <Graphics/Animations/Bone.h>

#include <Utils/Types/RawMesh.h>
#include <Utils/DebugDraw.h>
#include <Utils/FileSystem/PathDataAccessor.h>
#include <Utils/ECS/Transform3D.h>
#include <Utils/World/Scene.h>
#include <Utils/Common/Features.h>
#include <Utils/Events/Broadcaster.h>

#include <Codegen/Skeleton.generated.hpp>

#ifdef SR_UTILS_ASSIMP
    #include <assimp/scene.h>
    #include <assimp/postprocess.h>
    #include <assimp/Importer.hpp>
#endif

namespace SR_ANIMATIONS_NS {
    Skeleton::~Skeleton() {
        DisableDebug();
        m_bonesByName.clear();
    }

    Skeleton::Ptr Skeleton::ImportSkeletonFromRawMesh(const SR_HTYPES_NS::RawMesh& rawMesh) {
        auto&& pSkeleton = SR_UTILS_NS::Factory::Instance().Create<SR_ANIMATIONS_NS::Skeleton>();
        if (!pSkeleton) {
            SRHalt0();
            return nullptr;
        }

        SR_HTYPES_NS::RawMeshHolder skeletonHolder;

        skeletonHolder.SetRawMesh(rawMesh.GetThis().DynamicCast<SR_HTYPES_NS::RawMesh>());
        for (uint32_t meshId = 0; meshId < rawMesh.GetMeshesCount(); ++meshId) {
            if (rawMesh.HasBones(meshId)) {
                skeletonHolder.SetMeshId(meshId);
                break;
            }
        }

        pSkeleton->SetRawMesh(skeletonHolder);
        return pSkeleton;
    }

    void Skeleton::OnDestroy() {
        Super::OnDestroy();
    }

    Bone* Skeleton::AddBone(Bone* pParent, const SR_UTILS_NS::StringAtom name, bool recalculate) {
        if ((!pParent && m_rootBone) || (pParent && !m_rootBone)) {
            SRHalt0();
            return nullptr;
        }

        Bone::Ptr pBone = SRNew<Bone>();

        pBone->pRoot = (m_rootBone ? m_rootBone : pBone).Get();
        pBone->pParent = pParent;
        pBone->name = name;

        if (m_rootBone) {
            pParent->bones.emplace_back(pBone);
        }
        else {
            m_rootBone = pBone;
            m_rootBone->InitTreeIfNeed();
        }

        if (recalculate) {
            ReCalculateSkeleton();
        }

        return pBone.Get();
    }

    bool Skeleton::ReCalculateSkeleton() {
        m_bonesByName.clear();

        if (!GetRootBone() && !TryInitializeBonesFromMesh()) {
            SR_WARN("Skeleton::ReCalculateSkeleton() : root bone is nullptr!");
            return false;
        }

        if (GetRootBone()) {
            GetRootBone()->InitTreeIfNeed();
            GetRootBone()->SetGameObject(GetGameObject());
        }

        m_bonesByName.reserve(SR_HUMANOID_MAX_BONES);

        const SR_HTYPES_NS::Function<void(SR_ANIMATIONS_NS::Bone*)> processBone = [&](SR_ANIMATIONS_NS::Bone* pBone) {
        #ifdef SR_DEBUG
            if (m_bonesByName.count(pBone->name) == 1) {
                SR_INFO("Skeleton::ReCalculateSkeleton() : bone with name \"" + pBone->name.ToStringRef() + "\" already exists in hash table!");
            }
        #endif

            m_bonesByName.insert(std::make_pair(pBone->name, pBone));

            for (auto&& pSubBone : pBone->bones) {
                processBone(pSubBone.Get());
            }
        };

        processBone(GetRootBone().Get());

        return true;
    }

    Bone* Skeleton::GetAnimationBone(SR_UTILS_NS::StringAtom name) {
        SR_TRACY_ZONE;
        return GetBone(name);
    }

    Bone* Skeleton::GetBone(SR_UTILS_NS::StringAtom name) {
        SR_TRACY_ZONE;

        if (auto&& pRoot = GetRootBone()) {
            pRoot->InitTreeIfNeed();
        }

        auto&& pBoneIt = m_bonesByName.find(name);
        if (pBoneIt == m_bonesByName.end()) {
            return nullptr;
        }

        return pBoneIt->second;
    }

    void Skeleton::OnEnable() {
        m_prepareFrameSubscription = SR_UTILS_NS::Broadcaster::Instance().Subscribe(SR_UTILS_NS::Events::EVENT_ON_PREPARE_FRAME, std::bind(&Skeleton::UpdateBonesSSBO, this));
    }

    void Skeleton::OnDisable() {
        m_prepareFrameSubscription.Reset();
    }

    void Skeleton::OnPostLoad() {
        Super::OnPostLoad();
    }

    void Skeleton::OnAttached() {
        ReCalculateSkeleton();

        if (auto&& pScene = TryGetScene()) {
            auto&& pRenderScene = pScene->GetDataStorage().GetValue<RenderScenePtr>();
            if (pRenderScene) {
                pRenderScene->SetDirty();
            }
        }

        m_multiFrameSSBOResources = SR_UTILS_NS::Features::Instance().Enabled("MultiFrameSSBOResources", false);

        Super::OnAttached();
    }

    void Skeleton::LateUpdate() {
        if (m_bonesByName.empty()) { /// Update не должен вызываться, если кости ещё не загружены
            return;
        }

        m_dirtyMatrices = true;

        if (m_debugEnabled) {
            UpdateDebug();
        }
        else {
            DisableDebug();
        }

        Super::LateUpdate();
    }

    void Skeleton::CalculateTransforms() {
        SR_TRACY_ZONE;

        static const auto& defaultMesh = SR_HTYPES_NS::MeshSceneStructure::MeshData();
        auto&& skeleton = GetSkeletonRawMesh();
        auto&& mesh = skeleton.IsValidMeshId() ? skeleton.GetRawMesh()->GetMeshData(skeleton.GetMeshId()) : defaultMesh;

        if (!mesh.maxBoneId.has_value()) {
            m_matrices.clear();
            m_transforms.clear();
            m_indices.clear();
            return;
        }

        const uint32_t requiredSize = mesh.maxBoneId.value() + 1;
        m_matrices.resize(requiredSize);
        m_transforms.resize(requiredSize);
        m_indices.resize(requiredSize);

        m_hasInvalidBones = false;
        m_isNeedRecalcTransforms = false;

        uint32_t transformIndex = 0;

        for (auto&& [hashName, boneInfo] : mesh.bones) {
            const uint32_t boneId = boneInfo.boneId.value();
            auto&& pBone = GetBone(hashName);
            if (!pBone) {
                m_transforms[boneId] = nullptr;
                m_hasInvalidBones = true;
                m_indices[transformIndex++] = boneId;
                continue;
            }

            auto&& pGameObject = pBone->GetGameObject();
            m_transforms[boneId] = pGameObject ? SR_UTILS_NS::DynamicPointerCast<SR_UTILS_NS::Transform3D>(pGameObject->GetTransform()) : nullptr;

            m_hasInvalidBones |= !m_transforms[boneId];

            m_indices[transformIndex++] = boneId;
        }
    }

    void Skeleton::DisableDebug() {
        if (m_debugLines.empty()) {
            return;
        }

        for (auto&& [pBone, debugId] : m_debugLines) {
            SR_UTILS_NS::DebugOverlayDraw::Instance().DrawLine(debugId);
        }

        m_debugLines.clear();
    }

    void Skeleton::UpdateBonesSSBO() {
        SR_TRACY_ZONE;
        if (const auto ssbo = GetBonesSSBO(); ssbo != SR_ID_INVALID)  {
            if (auto&& matrices = GetMatrices(); !matrices.empty()) {
                GetPipeline()->UpdateSSBO(ssbo, (void*)matrices.data(), matrices.size() * sizeof(SR_MATH_NS::Matrix4x4));
            }
        }
    }

    void Skeleton::UpdateDebug() {
        if (!GetRootBone()) {
            DisableDebug();
            return;
        }

        for (auto&& [hashName, pBone] : m_bonesByName) {
            if (!pBone->pParent || pBone->pParent == pBone->pRoot) {
                continue;
            }

            if (m_debugLines.count(pBone) == 0) {
                m_debugLines.insert(std::make_pair(pBone, SR_ID_INVALID));
            }

            auto&& debugId = m_debugLines[pBone];

            auto&& pFromGameObject = GetBone(pBone->name)->GetGameObject();
            auto&& pToGameObject = GetBone(pBone->pParent->name)->GetGameObject();

            if (!pFromGameObject || !pToGameObject) {
                continue;
            }

            auto&& fromPos = pFromGameObject->GetTransform()->GetMatrix().GetTranslate();
            auto&& toPos = pToGameObject->GetTransform()->GetMatrix().GetTranslate();

            debugId = SR_UTILS_NS::DebugOverlayDraw::Instance().DrawLine(
                debugId,
                fromPos,
                toPos,
                SR_MATH_NS::FColor(38, 37, 45, 255)
            );
        }
    }

    const SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4>& Skeleton::GetOffsets() const noexcept {
        auto&& skeleton = GetSkeletonRawMesh();
        if (auto&& pRawMesh = skeleton.GetRawMesh()) {
            return pRawMesh->GetBoneOffsetMatrices(skeleton.GetMeshId());
        }
        const static SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4> defValue;
        return defValue;
    }

    const SR_UTILS_NS::Vector<SR_MATH_NS::Matrix4x4>& Skeleton::GetMatrices() noexcept {
        if (!m_dirtyMatrices) {
            return m_matrices;
        }

        SR_TRACY_ZONE;

        //bool hasDirty = m_isNeedRecalcTransforms;
        bool wasCalledRecalc = false;

        if (m_isNeedRecalcTransforms) {
            CalculateTransforms();
        }

        for (uint64_t i = 0; i < m_matrices.size(); ++i) {
            const uint32_t index = m_indices[i];

            if (!m_transforms[index]) SR_UNLIKELY_ATTRIBUTE {
                m_isNeedRecalcTransforms = true;

                if (!wasCalledRecalc) {
                    CalculateTransforms();
                    wasCalledRecalc = true;
                }

                if (!m_transforms[index]) {
                    static const auto identityMatrix = SR_MATH_NS::Matrix4x4::Identity();
                    m_matrices[index] = identityMatrix;
                    continue;
                }
            }

            auto&& pTransform = m_transforms[index].Get();
            m_matrices[index] = pTransform->GetMatrix();
        }

        m_dirtyMatrices = false;

        return m_matrices;
    }

    void Skeleton::OnRawMeshChanged() {
        m_isNeedRecalcTransforms = true;
        m_isOffsetsSSBODirty = true;
        m_isBonesSSBODirty = true;
        if (m_offsetsSSBO) {
            m_offsetsSSBO->GetRenderContext()->SetDirty();
        }
    }

    void Skeleton::SwitchDebug() {
        m_debugEnabled = !m_debugEnabled;
    }

    SR_HTYPES_NS::SharedPtr<Bone>& Skeleton::GetRootBone() noexcept {
        if (auto&& pParent = m_parent.Get()) {
            return pParent->GetRootBone();
        }
        return m_rootBone;
    }

    const SR_GRAPH_NS::RenderContext::Ptr& Skeleton::GetRenderContext() const noexcept {
        if (!m_renderContext) SR_UNLIKELY_ATTRIBUTE {
            SRAssert2(SR_THIS_THREAD, "Skeleton::GetPipeline() : SR_THIS_THREAD is nullptr!");
            m_renderContext = SR_THIS_THREAD->GetContext()->GetValue<SR_GRAPH_NS::RenderContext::Ptr>();
            SRAssert2(m_renderContext, "Failed to get render context from thread context!");
        }
        return m_renderContext;
    }

    const Pipeline::Ptr& Skeleton::GetPipeline() const noexcept {
        if (m_pipeline) SR_LIKELY_ATTRIBUTE {
            return m_pipeline;
        }

        SR_TRACY_ZONE;

        m_pipeline = GetRenderContext()->GetPipeline();
        SRAssert2(m_pipeline, "Skeleton::GetPipeline() : m_pipeline is nullptr!");

        return m_pipeline;
    }

    int32_t Skeleton::GetOffsetsSSBO() const noexcept {
        if (!m_offsetsSSBO || m_isOffsetsSSBODirty) {
            SR_TRACY_ZONE;
            auto&& offsets = GetOffsets();
            m_offsetsSSBO = SR_GRAPH_NS::SSBOInstance::Create<SR_MATH_NS::Matrix4x4>(offsets.size(), SSBOUsage::CPUToGPU);
            m_offsetsSSBO->UpdateSSBO(offsets.data());
            m_isOffsetsSSBODirty = false;
        }
        return m_offsetsSSBO->GetSSBO();
    }

    int32_t Skeleton::GetBonesSSBO() const noexcept {
        SR_TRACY_ZONE;

        const auto frame = m_multiFrameSSBOResources ? GetPipeline()->GetCurrentImageIndex() : 0;
        if (!m_bonesSSBO[frame] || m_isBonesSSBODirty) {
            m_isBonesSSBODirty = false;
            auto&& matrices = const_cast<Skeleton*>(this)->GetMatrices();
            if (!matrices.empty()) {
                m_bonesSSBO[frame] = SR_GRAPH_NS::SSBOInstance::Create<SR_MATH_NS::Matrix4x4>(matrices.size(), SSBOUsage::CPUToGPU);
            }
            else {
                SR_ERROR("Skeleton::GetBonesSSBO() : matrices are empty!");
                return SR_ID_INVALID;
            }
        }

        return m_bonesSSBO[frame]->GetSSBO();
    }

    bool Skeleton::TryInitializeBonesFromMesh() {
        SR_TRACY_ZONE;

        SR_HTYPES_NS::RawMesh::Ptr pRawMesh = GetSkeletonRawMesh().GetRawMesh();

        if (pRawMesh) {
        #ifdef SR_UTILS_ASSIMP
            const aiScene* pScene = static_cast<const aiScene*>(pRawMesh->GetAssimpScene());

            if (!pScene->mRootNode) {
                return false;
            }

            const SR_HTYPES_NS::Function<void(aiNode*, SR_ANIMATIONS_NS::Bone*)> processNode = [&](aiNode* node, SR_ANIMATIONS_NS::Bone* pBone) {
                if (std::string_view(node->mName.C_Str(), node->mName.length).find("_$AssimpFbx$_") != std::string_view::npos) {
                    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
                        processNode(node->mChildren[i], pBone);
                    }
                    return;
                }
                pBone = AddBone(pBone, node->mName.C_Str(), false);

                for (uint32_t i = 0; i < node->mNumChildren; ++i) {
                    processNode(node->mChildren[i], pBone);
                }
            };

            processNode(pScene->mRootNode, m_rootBone.Get());
        #endif

            return m_rootBone;
        }

        return false;
    }

    const SkeletonRig* Skeleton::GetRig() const noexcept {
        return m_rig.GetResource().Get();
    }

    const SR_HTYPES_NS::RawMeshHolder& Skeleton::GetSkeletonRawMesh() const {
        if (!m_rig.IsValid()) {
            return m_skeleton;
        }
        return m_rig.GetResource()->GetSkeleton();
    }

    void Skeleton::ForEachTransform(const SR_HTYPES_NS::Function<void(SR_UTILS_NS::Transform&)>& callback) {
        SR_TRACY_ZONE;
        if (m_isNeedRecalcTransforms) {
            CalculateTransforms();
        }
        for (auto&& pTransform : m_transforms) {
            if (pTransform) {
                callback(*pTransform);
            }
        }
    }

    void Skeleton::ForEachBone(const SR_UTILS_NS::Function<void(Bone&)>& callback) {
        SR_TRACY_ZONE;

        if (auto&& pParent = m_parent.Get()) {
            pParent->ForEachBone(callback);
        }
        else if (GetRootBone()) {
            const SR_HTYPES_NS::Function<void(SR_ANIMATIONS_NS::Bone*)> processBone = [&](SR_ANIMATIONS_NS::Bone* pBone) {
                callback(*pBone);
                for (auto&& pSubBone : pBone->bones) {
                    processBone(pSubBone.Get());
                }
            };

            processBone(GetRootBone().Get());
        }
    }
}
