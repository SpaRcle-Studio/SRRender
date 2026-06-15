//
// Created by Monika on 14.06.2026.
//

#include <Graphics/Animations/SkeletonRig.h>

#include <Utils/FileSystem/FileDialog.h>
#include <Utils/Types/RawMesh.h>
#include <Utils/Resources/ResourceRef.h>

#include <Codegen/SkeletonRig.generated.hpp>

namespace SR_ANIMATIONS_NS {
    void SkeletonRig::AutoRemapHumanoidBonesImpl(const SR_HTYPES_NS::RawMesh& rawMesh, uint32_t index) {
        SR_TRACY_ZONE;

        const auto& bones = rawMesh.GetBones(index);

        for (const auto& [boneName, boneIndex] : bones) {
            if (boneName.empty()) {
                continue;
            }

            HumanoidBoneType humanoidBoneType = ExtractHumanoidBoneType(boneName);
            if (humanoidBoneType == HumanoidBoneType::Unknown) {
                continue;
            }
            auto&& boneInfo = m_mapping[SR_UTILS_NS::EnumReflector::ToStringAtom(humanoidBoneType)].bones.emplace_back();
            boneInfo.name = boneName;
            boneInfo.index = boneIndex;
        }
    }

    SR_UTILS_NS::StringAtom SkeletonRig::RetargetBone(SR_UTILS_NS::StringAtom name) const {
        SR_TRACY_ZONE;

        for (const auto& [boneName, boneChain] : m_mapping) {
            for (const auto& boneInfo : boneChain.bones) {
                if (boneInfo.name == name) {
                    return boneName;
                }
            }
        }

        return SR_UTILS_NS::StringAtom();
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

    void SkeletonRig::AutoRemapHumanoidBones() {
        SR_TRACY_ZONE;

        if (m_skeletonType != SkeletonType::Humanoid) {
            SR_ERROR("SkeletonRig::AutoRemapHumanoidBones() : skeleton type is not humanoid!");
            return;
        }

        auto&& resourcesPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath();
        if (auto&& path = SR_UTILS_NS::FileDialog::Instance().OpenDialog(resourcesPath.ToString(), { { "Any skeleton", "prefab,pmx,fbx,obj,blend,dae,abc,stl,ply,glb,gltf,x3d,sfg,bvh,3ds,gltf" } }); !path.IsEmpty()) {
            SR_UTILS_NS::ResourceRef<SR_HTYPES_NS::RawMesh> rawMesh = path.RemoveSubPath(resourcesPath);
            if (auto&& pRawMesh = rawMesh.GetResource()) {
                for (uint32_t i = 0; i < pRawMesh->GetMeshesCount(); ++i) {
                    if (pRawMesh->GetBones(i).empty()) {
                        continue;
                    }
                    AutoRemapHumanoidBonesImpl(*pRawMesh, i);
                    break;
                }
            }
        }
    }
}
