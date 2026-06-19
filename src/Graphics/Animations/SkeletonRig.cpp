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

        for (const auto& [boneName, boneInfo] : bones) {
            SR_UTILS_NS::StringAtom workingBoneName = boneName;
            if (workingBoneName.empty() || !boneInfo.boneId.has_value()) {
                continue;
            }

            if (m_autoRigRules.IsBoneIgnored(workingBoneName)) {
                continue;
            }

            for (const auto& prefix : m_autoRigRules.removePrefixes) {
                if (workingBoneName.ToStringView().starts_with(prefix.ToStringView())) {
                    workingBoneName = SR_UTILS_NS::StringAtom(workingBoneName.ToStringView().substr(prefix.ToStringView().size()));
                    break;
                }
            }

            HumanoidBoneType humanoidBoneType = ExtractHumanoidBoneType(workingBoneName);
            if (humanoidBoneType == HumanoidBoneType::Unknown) {
                continue;
            }
            auto&& rigBoneInfo = m_mapping[SR_UTILS_NS::EnumReflector::ToStringAtom(humanoidBoneType)].bones.emplace_back();
            rigBoneInfo.name = boneName;
            rigBoneInfo.index = boneInfo.boneId.value();

            if (boneInfo.nodeIndex.has_value()) {
                auto&& node = rawMesh.GetSceneStructure().GetNodeByIndex(boneInfo.nodeIndex.value());
                rigBoneInfo.bindTranslation = node.localTransform.translation;
                rigBoneInfo.bindRotation = node.localTransform.rotation;
                rigBoneInfo.bindScale = node.localTransform.scale;
            }
            else {
                SR_WARN("SkeletonRig::AutoRemapHumanoidBonesImpl() : bone node index is not set! Bone name: {}", boneName);
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

    TranslationRetargetMode SkeletonRig::GetTranslationRetargetMode(SR_UTILS_NS::StringAtom humanoidKey) const noexcept {
        if (auto&& it = m_translationRetargetModes.find(humanoidKey); it != m_translationRetargetModes.end()) {
            return it->second;
        }

        if (humanoidKey == SR_UTILS_NS::StringAtom("Hips")) {
            return TranslationRetargetMode::OrientAndScale;
        }

        return TranslationRetargetMode::Skeleton;
    }

    bool SkeletonRig::TryGetRetargetPoseLocal(SR_UTILS_NS::StringAtom boneName, SkeletonRigPoseBone& outPose) const noexcept {
        for (const auto& pose : m_retargetPoseLocal) {
            if (pose.name == boneName) {
                outPose = pose;
                return true;
            }
        }
        return false;
    }

    void SkeletonRig::InitializeRetargetPoseFromBind() {
        SR_TRACY_ZONE;

        m_retargetPoseLocal.clear();

        auto&& pRawMesh = m_skeleton.GetRawMesh();
        if (!pRawMesh) {
            SR_WARN("SkeletonRig::InitializeRetargetPoseFromBind() : raw mesh is nullptr!");
            return;
        }

        const auto meshId = m_skeleton.GetMeshId();
        if (meshId == SR_ID_INVALID) {
            SR_WARN("SkeletonRig::InitializeRetargetPoseFromBind() : mesh id is invalid!");
            return;
        }

        const auto& bones = pRawMesh->GetMeshData(meshId).bones;
        m_retargetPoseLocal.reserve(bones.size());

        for (const auto& [boneName, boneInfo] : bones) {
            if (boneName.empty() || !boneInfo.nodeIndex.has_value()) {
                continue;
            }

            const auto& node = pRawMesh->GetSceneStructure().GetNodeByIndex(boneInfo.nodeIndex.value());

            auto&& pose = m_retargetPoseLocal.emplace_back();
            pose.name = boneName;
            pose.translation = node.localTransform.translation;
            pose.rotation = node.localTransform.rotation;
            pose.scale = node.localTransform.scale;
        }
    }
}
