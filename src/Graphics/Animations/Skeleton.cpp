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
            m_rootBone->SetSkeleton(this);
            m_rootBone->InitTreeIfNeed();
        }

        if (recalculate) {
            ReCalculateSkeleton();
        }

        return pBone.Get();
    }

    bool Skeleton::ReCalculateSkeleton() {
        m_bonesByName.clear();
        m_bonesByIndex.clear();

        if (!m_rootBone && !TryInitializeBonesFromMesh()) {
            SR_WARN("Skeleton::ReCalculateSkeleton() : root bone is nullptr!");
            return false;
        }

        m_rootBone->InitTreeIfNeed();

        if (m_rootBone) {
            m_rootBone->gameObject = GetGameObject();
        }

        m_bonesByName.reserve(SR_HUMANOID_MAX_BONES);
        m_bonesByIndex.reserve(SR_HUMANOID_MAX_BONES);

        const SR_HTYPES_NS::Function<void(SR_ANIMATIONS_NS::Bone*)> processBone = [&](SR_ANIMATIONS_NS::Bone* pBone) {
        #ifdef SR_DEBUG
            if (m_bonesByName.count(pBone->name) == 1) {
                SR_WARN("Skeleton::ReCalculateSkeleton() : bone with name \"" + pBone->name.ToStringRef() + "\" already exists in hash table!");
            }
        #endif

            m_bonesByIndex.emplace_back(pBone);
            m_bonesByName.insert(std::make_pair(pBone->name, pBone));

            for (auto&& pSubBone : pBone->bones) {
                processBone(pSubBone.Get());
            }
        };

        processBone(m_rootBone.Get());

        return true;
    }

    Bone* Skeleton::GetBone(SR_UTILS_NS::StringAtom name) {
        if (m_rootBone) {
            m_rootBone->InitTreeIfNeed();
        }

        auto&& pBoneIt = m_bonesByName.find(name);
        if (pBoneIt == m_bonesByName.end()) {
            return nullptr;
        }

        if (!pBoneIt->second->gameObject && !pBoneIt->second->hasError && !pBoneIt->second->Initialize()) {
            SR_WARN("Skeleton::GetBone() : failed to find bone game object!\n\tName: {}", pBoneIt->second->name);
        }

        return pBoneIt->second;
    }

    Bone* Skeleton::GetBoneByIndex(uint16_t index) const {
        if (m_rootBone) {
            m_rootBone->InitTreeIfNeed();
        }

        if (index >= m_bonesByIndex.size()) SR_UNLIKELY_ATTRIBUTE {
            return nullptr;
        }

        if (!m_bonesByIndex[index]->gameObject && !m_bonesByIndex[index]->hasError) SR_UNLIKELY_ATTRIBUTE {
            if (!m_bonesByIndex[index]->Initialize()) {
                return nullptr;
            }
        }

        return m_bonesByIndex[index];
    }

    Bone* Skeleton::TryGetBone(SR_UTILS_NS::StringAtom name) {
        if (m_rootBone) {
            m_rootBone->InitTreeIfNeed();
        }

        auto&& pBoneIt = m_bonesByName.find(name);
        if (pBoneIt == m_bonesByName.end()) {
            return nullptr;
        }

        if (!pBoneIt->second->gameObject && !pBoneIt->second->hasError) {
            pBoneIt->second->Initialize();
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
        if (m_rootBone) {
            m_rootBone->SetSkeleton(this);
        }
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

        auto&& optimizedBones = GetOptimizedBones();

        m_matrices.resize(optimizedBones.size());
        m_transforms.resize(optimizedBones.size());
        m_indices.resize(optimizedBones.size());

        m_hasInvalidBones = false;
        m_isNeedRecalcTransforms = false;

        uint32_t transformIndex = 0;

        for (auto&& [hashName, index] : optimizedBones) {
            auto&& pBone = GetBone(hashName);
            if (!pBone) {
                m_transforms[index] = nullptr;
                m_hasInvalidBones = true;
                m_indices[transformIndex++] = index;
                continue;
            }

            auto&& pGameObject = pBone->gameObject;

            if (pGameObject || pBone->hasError || pBone->Initialize()) {
                m_transforms[index] = pGameObject ? pGameObject->GetTransform().DynamicCast<SR_UTILS_NS::Transform3D>() : nullptr;
            }
            m_hasInvalidBones |= !m_transforms[index];

            m_indices[transformIndex++] = index;
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
        if (!m_rootBone) {
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

            auto&& fromGameObject = GetBone(pBone->name);
            auto&& toGameObject = GetBone(pBone->pParent->name);

            if (!fromGameObject->gameObject || !toGameObject->gameObject) {
                continue;
            }

            auto&& fromPos = fromGameObject->gameObject->GetTransform()->GetMatrix().GetTranslate();
            auto&& toPos = toGameObject->gameObject->GetTransform()->GetMatrix().GetTranslate();

            debugId = SR_UTILS_NS::DebugOverlayDraw::Instance().DrawLine(
                debugId,
                fromPos,
                toPos,
                SR_MATH_NS::FColor(38, 37, 45, 255)
            );
        }
    }

    uint64_t Skeleton::GetBoneIndex(SR_UTILS_NS::StringAtom name) {
        for (uint64_t i = 0; i < m_bonesByIndex.size(); ++i) {
            if (m_bonesByIndex[i]->name == name) {
                return i;
            }
        }

        return SR_ID_INVALID;
    }

    const SR_MATH_NS::Matrix4x4& Skeleton::GetMatrixByIndex(uint16_t index) noexcept {
        static SR_MATH_NS::Matrix4x4 identityMatrix = SR_MATH_NS::Matrix4x4().Identity();

        if (index >= m_bonesByIndex.size()) {
            return identityMatrix;
        }

        return m_matrices[index];
    }

    const std::vector<SR_MATH_NS::Matrix4x4>& Skeleton::GetOffsets() const noexcept {
        if (auto&& pRawMesh = m_skeleton.GetRawMesh()) {
            return pRawMesh->GetBoneOffsets();
        }
        const static std::vector<SR_MATH_NS::Matrix4x4> defValue;
        return defValue;
    }

    const std::vector<SR_MATH_NS::Matrix4x4>& Skeleton::GetMatrices() noexcept {
        if (!m_dirtyMatrices) {
            return m_matrices;
        }

        SR_TRACY_ZONE;

        //bool hasDirty = m_isNeedRecalcTransforms;
        bool wasCalledRecalc = false;

        if (m_isNeedRecalcTransforms) {
            CalculateTransforms();
        }

        const uint64_t optimizedBonesSize = GetOptimizedBones().size();

        for (uint64_t i = 0; i < optimizedBonesSize; ++i) {
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

            //if (hasDirty) {
            //    m_matrices[index] = pTransform->GetMatrix();
            //    continue;
            //}

           // if (pTransform->IsDirty()) {
           //     hasDirty = true;
            m_matrices[index] = pTransform->GetMatrix();
           // }
        }

        m_dirtyMatrices = false;

        return m_matrices;
    }

    const ska::flat_hash_map<SR_UTILS_NS::StringAtom, uint16_t>& Skeleton::GetOptimizedBones() const noexcept {
        if (auto&& pRawMesh = m_skeleton.GetRawMesh()) {
            return pRawMesh->GetOptimizedBones();
        }
        static ska::flat_hash_map<SR_UTILS_NS::StringAtom, uint16_t> defValue;
        return defValue;
    }

    void Skeleton::OnRawMeshChanged() {
        m_isNeedRecalcTransforms = true;
        m_isOffsetsSSBODirty = true;
        m_isBonesSSBODirty = true;
        if (m_offsetsSSBO) {
            m_offsetsSSBO->GetRenderContext()->SetDirty();
        }
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

        if (m_customHierarchy) {
            return false;
        }

        SR_HTYPES_NS::RawMesh::Ptr pRawMesh = m_boneHierarchy.GetRawMesh();
        if (!pRawMesh) {
            pRawMesh = m_skeleton.GetRawMesh();
        }

        if (pRawMesh) {
        #ifdef SR_UTILS_ASSIMP
            const aiScene* pScene = static_cast<const aiScene*>(pRawMesh->GetAssimpScene());

            if (!pScene->mRootNode) {
                return false;
            }

            const SR_HTYPES_NS::Function<void(aiNode*, SR_ANIMATIONS_NS::Bone*)> processNode = [&](aiNode* node, SR_ANIMATIONS_NS::Bone* pBone) {
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
}
