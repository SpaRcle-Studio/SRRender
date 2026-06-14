//
// Created by Monika on 14.06.2026.
//

#include <Graphics/Animations/SkeletonRetarget.h>

#include <Utils/FileSystem/FileDialog.h>
#include <Utils/Types/RawMesh.h>
#include <Utils/Resources/ResourceRef.h>

#include <Codegen/SkeletonRetarget.generated.hpp>

namespace SR_ANIMATIONS_NS {
    void SkeletonRetargetProfile::AutoRemapHumanoidBonesImpl(const SR_HTYPES_NS::RawMesh& rawMesh, uint32_t index) {
        SR_TRACY_ZONE;

        const auto& bones = rawMesh.GetBones(index);

        for (const auto& [boneName, boneIndex] : bones) {
            HumanoidBoneType humanoidBoneType = ExtractHumanoidBoneType(boneName);
            if (humanoidBoneType == HumanoidBoneType::Unknown) {
                continue;
            }
            m_mapping[humanoidBoneType].bones.emplace_back(boneName);
        }
    }

    void SkeletonRetargetProfile::AutoRemapHumanoidBones() {
        SR_TRACY_ZONE;

        if (m_skeletonType != SkeletonType::Humanoid) {
            SR_ERROR("SkeletonRetargetProfile::AutoRemapHumanoidBones() : skeleton type is not humanoid!");
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
