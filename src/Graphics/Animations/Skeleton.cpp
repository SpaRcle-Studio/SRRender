//
// Created by Monika on 11.01.2023.
//

#include <Graphics/Animations/Skeleton.h>

#include <Graphics/Render/RenderScene.h>
#include <Graphics/Utils/MeshUtils.h>

#include <Utils/Types/RawMesh.h>
#include <Utils/DebugDraw.h>

#include <Codegen/Skeleton.generated.hpp>

namespace SR_ANIMATIONS_NS {
    Skeleton::~Skeleton() {
        DisableDebug();
        m_bonesByName.clear();
    }

    void Skeleton::OnDestroy() {
        Super::OnDestroy();
        GetThis().AutoFree([](auto&& pData) {
            delete pData;
        });
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

        if (!m_rootBone) {
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
            SR_WARN("Skeleton::GetBone() : failed to find bone game object!\n\tName: " + pBoneIt->second->name);
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

        Super::OnAttached();
    }

    void Skeleton::Update(float_t dt) {
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

        Super::Update(dt);
    }

    void Skeleton::DisableDebug() {
        if (m_debugLines.empty()) {
            return;
        }

        for (auto&& [pBone, debugId] : m_debugLines) {
            SR_UTILS_NS::DebugDraw::Instance().DrawLine(debugId);
        }

        m_debugLines.clear();
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

            debugId = SR_UTILS_NS::DebugDraw::Instance().DrawLine(
                    debugId,
                    fromPos,
                    toPos,
                    SR_MATH_NS::FColor(38, 37, 45, 255)
            );
        }
    }

    SR_UTILS_NS::Transform* Skeleton::GetTransformByIndex(uint16_t index) noexcept {
        if (index >= m_bonesByIndex.size()) {
            return nullptr;
        }

        auto&& pBone = m_bonesByIndex[index];
        auto&& pGameObject = pBone->gameObject;

        if (!pGameObject && !pBone->hasError && !pBone->Initialize()) {
            return nullptr;
        }

        if (pGameObject) {
            return pGameObject->GetTransform().Get();
        }
        return nullptr;
    }

    uint64_t Skeleton::GetBoneIndex(SR_UTILS_NS::StringAtom name) {
        for (uint64_t i = 0; i < m_bonesByIndex.size(); ++i) {
            if (m_bonesByIndex[i]->name == name) {
                return i;
            }
        }

        return SR_ID_INVALID;
    }

    void Skeleton::CalculateMatrices() {
        if (!m_dirtyMatrices) {
            return;
        }

        SR_TRACY_ZONE;

        m_matrices.resize(m_bonesByIndex.size());

        for (uint16_t i = 0; i < m_bonesByIndex.size(); ++i) {
            auto&& pBone = m_bonesByIndex[i];
            auto&& pGameObject = pBone->gameObject;

            if (!pGameObject && !pBone->hasError && !pBone->Initialize()) {
                continue;
            }

            if (pGameObject) {
                m_matrices[i] = pGameObject->GetTransform()->GetMatrix();
            }
        }

        m_dirtyMatrices = false;
    }

    const SR_MATH_NS::Matrix4x4& Skeleton::GetMatrixByIndex(uint16_t index) noexcept {
        static SR_MATH_NS::Matrix4x4 identityMatrix = SR_MATH_NS::Matrix4x4().Identity();

        if (index >= m_bonesByIndex.size()) {
            return identityMatrix;
        }

        return m_matrices[index];
    }

    const std::vector<SR_MATH_NS::Matrix4x4>& Skeleton::GetMatrices() noexcept {
        if (!m_dirtyMatrices) {
            return m_matrices;
        }

        SR_TRACY_ZONE;

        m_matrices.resize(m_optimizedBones.size());

        for (auto&& [hashName, index] : m_optimizedBones) {
            auto&& pBone = GetBone(hashName);
            auto&& pGameObject = pBone->gameObject;

            if (!pGameObject && !pBone->hasError && !pBone->Initialize()) {
                continue;
            }

            if (pGameObject) {
                m_matrices[index] = pGameObject->GetTransform()->GetMatrix();
            }
        }

        m_dirtyMatrices = false;

        return m_matrices;
    }

    void Skeleton::SetOptimizedBones(const ska::flat_hash_map<SR_UTILS_NS::StringAtom, uint16_t>& bones) {
        if (m_optimizedBones.empty()) {
            m_optimizedBones = bones;
        }
    }

    void Skeleton::SetBonesOffsets(const std::vector<SR_MATH_NS::Matrix4x4>& offsets) {
        if (m_skeletonOffsets.empty()) {
            m_skeletonOffsets = offsets;
            m_skeletonOffsets.resize(m_skeletonOffsets.size());
        }
    }

    void Skeleton::ResetSkeleton() {
        m_optimizedBones.clear();
        m_skeletonOffsets.clear();
    }
}
