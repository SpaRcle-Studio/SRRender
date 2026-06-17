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
            if (boneName.empty() || !boneInfo.boneId.has_value()) {
                continue;
            }

            if (m_autoRigRules.IsBoneIgnored(boneName)) {
                continue;
            }

            HumanoidBoneType humanoidBoneType = ExtractHumanoidBoneType(boneName);
            if (humanoidBoneType == HumanoidBoneType::Unknown) {
                continue;
            }
            auto&& rigBoneInfo = m_mapping[SR_UTILS_NS::EnumReflector::ToStringAtom(humanoidBoneType)].bones.emplace_back();
            rigBoneInfo.name = boneName;
            rigBoneInfo.index = boneInfo.boneId.value();

            if (boneInfo.nodeIndex.has_value()) {
                auto&& node = rawMesh.GetSceneStructure().GetNodeByIndex(boneInfo.nodeIndex.value());
                rigBoneInfo.bindTranslation = node.transform.translation;
                rigBoneInfo.bindRotation = node.transform.rotation;
                rigBoneInfo.bindScale = node.transform.scale;
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

        auto&& resourcesPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath();
        if (auto&& path = SR_UTILS_NS::FileDialog::Instance().OpenDialog(resourcesPath.ToString(), { { "Any skeleton", "prefab,pmx,fbx,obj,blend,dae,abc,stl,ply,glb,gltf,x3d,sfg,bvh,3ds,gltf" } }); !path.IsEmpty()) {
            SR_UTILS_NS::ResourceRef<SR_HTYPES_NS::RawMesh> rawMesh = path.RemoveSubPath(resourcesPath);
            if (auto&& pRawMesh = rawMesh.GetResource()) {
                for (uint32_t i = 0; i < pRawMesh->GetMeshesCount(); ++i) {
                    if (pRawMesh->GetMeshData(i).bones.empty()) {
                        continue;
                    }
                    AutoRemapHumanoidBonesImpl(*pRawMesh, i);
                    break;
                }
            }
        }
    }
}
