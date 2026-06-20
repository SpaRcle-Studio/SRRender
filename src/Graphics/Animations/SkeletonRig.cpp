//
// Created by Monika on 14.06.2026.
//

#include <Graphics/Animations/SkeletonRig.h>

#include <Utils/FileSystem/FileDialog.h>
#include <Utils/Types/RawMesh.h>
#include <Utils/Resources/ResourceRef.h>

#include <Codegen/SkeletonRig.generated.hpp>

namespace SR_ANIMATIONS_NS {
    bool SkeletonAutoRigRules::IsBoneIgnored(SR_UTILS_NS::StringAtom name) const {
        if (name.empty()) {
            return true;
        }
        if (std::ranges::any_of(ignoreBonesStartsWith, [&](const auto& prefix) { return name.ToStringView().starts_with(prefix.ToStringView()); })) {
            return true;
        }
        if (std::ranges::any_of(ignoreBonesContains, [&](const auto& substring) { return name.ToStringView().find(substring.ToStringView()) != std::string::npos; })) {
            return true;
        }
        if (std::ranges::any_of(ignoreBonesEndsWith, [&](const auto& suffix) { return name.ToStringView().ends_with(suffix.ToStringView()); })) {
            return true;
        }
        return false;
    }

    void SkeletonRig::AutoRemapHumanoidBonesImpl(const SR_HTYPES_NS::RawMesh& rawMesh, uint32_t index) {
        SR_TRACY_ZONE;

        const auto& bones = rawMesh.GetMeshData(index).bones;

        SR_HTYPES_NS::FlatHashSet<HumanoidBoneType> mappedBones;

        auto&& sceneStructure = rawMesh.GetSceneStructure();
        sceneStructure.ForEachNode(true, [&](const SR_HTYPES_NS::MeshSceneStructure::SceneNode& sceneNode) {
            auto&& pBoneInfoIt = bones.find(sceneNode.name);
            if (pBoneInfoIt == bones.end()) {
                return;
            }
            const auto& boneName = sceneNode.name;
            const auto& boneInfo = pBoneInfoIt->second;

            SR_UTILS_NS::StringAtom workingBoneName = boneName;
            if (workingBoneName.empty() || !boneInfo.boneId.has_value()) {
                return;
            }

            if (m_autoRigRules.IsBoneIgnored(workingBoneName)) {
                return;
            }

            for (const auto& prefix : m_autoRigRules.removePrefixes) {
                if (workingBoneName.ToStringView().starts_with(prefix.ToStringView())) {
                    workingBoneName = SR_UTILS_NS::StringAtom(workingBoneName.ToStringView().substr(prefix.ToStringView().size()));
                    break;
                }
            }

            HumanoidBoneType humanoidBoneType = ExtractHumanoidBoneType(workingBoneName, mappedBones);
            if (humanoidBoneType == HumanoidBoneType::Unknown) {
                return;
            }
            auto&& rigBoneInfo = m_mapping[SR_UTILS_NS::EnumReflector::ToStringAtom(humanoidBoneType)].bones.emplace_back();
            rigBoneInfo.name = boneName;
            rigBoneInfo.index = boneInfo.boneId.value();
        });

        /// разделяем цепочку спины на Spine, Chest, UpperChest
        /// обязательно должен быть Spine, затем обязательно UpperChest, и только потом Chest, если они есть
        auto&& pSpineChainIt = m_mapping.find(SR_UTILS_NS::EnumReflector::ToStringAtom(HumanoidBoneType::Spine));
        if (pSpineChainIt != m_mapping.end()) {
            SkeletonRigBoneChain spineChain = pSpineChainIt->second;
            const auto count = static_cast<uint32_t>(spineChain.bones.size());
            if (count >= 1) {
                m_mapping[SR_UTILS_NS::EnumReflector::ToStringAtom(HumanoidBoneType::Spine)].bones = { spineChain.bones.front() };
            }

            if (count >= 2) {
                m_mapping[SR_UTILS_NS::EnumReflector::ToStringAtom(HumanoidBoneType::UpperChest)].bones = { spineChain.bones.back() };
            }

            if (count >= 3) {
                auto& chestChain = m_mapping[SR_UTILS_NS::EnumReflector::ToStringAtom(HumanoidBoneType::Chest)];
                chestChain.bones.assign(spineChain.bones.begin() + 1, spineChain.bones.end() - 1);
            }
        }

        auto&& pHipsIt = m_mapping.find(SR_UTILS_NS::EnumReflector::ToStringAtom(HumanoidBoneType::Hips));
        if (pHipsIt != m_mapping.end() && pHipsIt->second.bones.size() > 1) {
            /// Если обнаружили несколько pelvis/hips костей, то вероятно они имеют направления, и у них есть общая родительская кость, которая может быть корнем рига.
            /// В этом случае добавляем эту общую родительскую кость вместо текущих pelvis/hips костей
            const auto& hipsBone = pHipsIt->second.bones.front();
            if (hipsBone.index != SR_ID_INVALID) {
                const auto& hipsBoneInfo = rawMesh.GetBoneInfo(index, hipsBone.name);
                if (hipsBoneInfo.nodeIndex.has_value()) {
                    auto&& hipsNode = sceneStructure.GetNodeByIndex(hipsBoneInfo.nodeIndex.value());
                    if (hipsNode.parent.has_value()) {
                        const auto& parentNode = sceneStructure.GetNodeByIndex(hipsNode.parent.value());
                        auto&& pParentBoneInfoIt = bones.find(parentNode.name);
                        if (pParentBoneInfoIt != bones.end() && pParentBoneInfoIt->second.boneId.has_value()) {
                            const auto& parentBone = pParentBoneInfoIt->second;
                            auto& hipsChain = pHipsIt->second;
                            hipsChain.bones.clear();
                            auto& newBone = hipsChain.bones.emplace_back();
                            newBone.name = parentNode.name;
                            newBone.index = parentBone.boneId.value();
                        }
                    }
                }
            }
        }
    }

    const SkeletonRigBoneChain* SkeletonRig::RetargetBone(SR_UTILS_NS::StringAtom name, SR_UTILS_NS::StringAtom& outName) const {
        SR_TRACY_ZONE;

        for (const auto& [boneName, boneChain] : m_mapping) {
            for (const auto& boneInfo : boneChain.bones) {
                if (boneInfo.name == name) {
                    outName = boneName;
                    return &boneChain;
                }
            }
        }

        return nullptr;
    }

    SR_UTILS_NS::StringAtom SkeletonRig::GetBoneName(SR_UTILS_NS::StringAtom name) const {
        SR_TRACY_ZONE;

        if (auto&& pIt = m_mapping.find(name); pIt != m_mapping.end()) {
            if (!pIt->second.bones.empty()) {
                return pIt->second.bones.front().name;
            }
        }

        return SR_UTILS_NS::StringAtom();
    }

    uint32_t SkeletonRig::GetBoneIndex(SR_UTILS_NS::StringAtom name) const {
        SR_TRACY_ZONE;

        if (auto&& pIt = m_mapping.find(name); pIt != m_mapping.end()) {
            if (!pIt->second.bones.empty()) {
                return pIt->second.bones.front().index;
            }
        }

        return SR_ID_INVALID;
    }

    const SkeletonRigBoneChain* SkeletonRig::GetBoneChain(SR_UTILS_NS::StringAtom name) const {
        SR_TRACY_ZONE;

        if (auto&& pIt = m_mapping.find(name); pIt != m_mapping.end()) {
            return &pIt->second;
        }

        return nullptr;
    }

    void SkeletonRig::AutoRemapHumanoidBones() {
        SR_TRACY_ZONE;

        if (m_skeletonType != SkeletonType::Humanoid) {
            SR_ERROR("SkeletonRig::AutoRemapHumanoidBones() : skeleton type is not humanoid!");
            return;
        }

        m_mapping.clear();

        if (auto&& pRawMesh = m_skeleton.GetRawMesh()) {
            AutoRemapHumanoidBonesImpl(*pRawMesh, m_skeleton.GetMeshId());
        }
    }

    bool SkeletonRig::TryGetRetargetPoseLocal(SR_UTILS_NS::StringAtom boneName, SR_MATH_NS::DecomposedMatrix& outPose) const noexcept {
        auto&& pSkeleton = m_skeleton.GetRawMesh();
        if (!pSkeleton) {
            return false;
        }

        auto&& meshData = pSkeleton->GetMeshData(m_skeleton.GetMeshId());
        auto&& pBoneInfoIt = meshData.bones.find(boneName);
        if (pBoneInfoIt == meshData.bones.end()) {
            return false;
        }

        if (pBoneInfoIt->second.nodeIndex.has_value()) {
            auto&& node = pSkeleton->GetSceneStructure().GetNodeByIndex(pBoneInfoIt->second.nodeIndex.value());
            outPose = node.localTransform;
            return true;
        }
        return false;
    }
}
