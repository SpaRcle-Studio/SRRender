//
// Created by Monika on 15.06.2026.
//

#include <Graphics/Animations/HumanoidBoneType.h>

namespace SR_ANIMATIONS_NS {
    HumanoidBoneType TryExtractSidedHumanoidBone(const SR_UTILS_NS::String& name, bool leftSide) {
        /// UpperLeg, LowerLeg, Foot, Toes,
        if (((name.contains("upper") || name.contains("up")) && name.contains("leg")) || name.contains("thigh")) {
            return leftSide ? HumanoidBoneType::LeftUpperLeg : HumanoidBoneType::RightUpperLeg;
        }
        else if (name.contains("leg") || name.contains("calf") || name.contains("shin")) {
            return leftSide ? HumanoidBoneType::LeftLowerLeg : HumanoidBoneType::RightLowerLeg;
        }
        else if (name.contains("foot")) {
            return leftSide ? HumanoidBoneType::LeftFoot : HumanoidBoneType::RightFoot;
        }
        else if (name.contains("toe") || name.contains("ball")) {
            return leftSide ? HumanoidBoneType::LeftToes : HumanoidBoneType::RightToes;
        }

        /// Thumb, Index, Middle, Ring, Little
        if (name.contains("thumb")) {
            return leftSide ? HumanoidBoneType::LeftThumbProximal : HumanoidBoneType::RightThumbProximal;
        }
        else if (name.contains("index")) {
            return leftSide ? HumanoidBoneType::LeftIndexProximal : HumanoidBoneType::RightIndexProximal;
        }
        else if (name.contains("middle")) {
            return leftSide ? HumanoidBoneType::LeftMiddleProximal : HumanoidBoneType::RightMiddleProximal;
        }
        else if (name.contains("ring")) {
            return leftSide ? HumanoidBoneType::LeftRingProximal : HumanoidBoneType::RightRingProximal;
        }
        else if (name.contains("little") || name.contains("pinky")) {
            return leftSide ? HumanoidBoneType::LeftLittleProximal : HumanoidBoneType::RightLittleProximal;
        }

        /// Shoulder, UpperArm, LowerArm, Hand,
        if (name.contains("shoulder") || name.contains("clavicle")) {
            return leftSide ? HumanoidBoneType::LeftShoulder : HumanoidBoneType::RightShoulder;
        }
        else if (name.contains("arm") && (name.contains("upper") || name.contains("up") || name.contains("forearm"))) {
            return leftSide ? HumanoidBoneType::LeftUpperArm : HumanoidBoneType::RightUpperArm;
        }
        else if (name.contains("arm")) {
            return leftSide ? HumanoidBoneType::LeftLowerArm : HumanoidBoneType::RightLowerArm;
        }
        else if (name.contains("hand")) {
            return leftSide ? HumanoidBoneType::LeftHand : HumanoidBoneType::RightHand;
        }

        return HumanoidBoneType::Unknown;
    }

    HumanoidBoneType ExtractHumanoidBoneType(SR_UTILS_NS::StringAtom name) {
        SR_TRACY_ZONE;

        SR_THREAD_LOCAL static SR_UTILS_NS::String workingName;
        workingName = name.ToStringView();
        workingName.ToLowerInPlace();

        if (workingName.contains("mixamo")) {
            if (auto separatorIndex = workingName.find(':'); separatorIndex != SR_UTILS_NS::String::npos) {
                workingName.SubStrInPlace(separatorIndex + 1);
            }
        }

        if (workingName.contains("hips") || workingName.contains("pelvis")) {
            return HumanoidBoneType::Hips;
        }
        else if (workingName == "spine") {
            return HumanoidBoneType::Spine;
        }
        else if (workingName.contains("chest") || workingName.contains("spine")) {
            return HumanoidBoneType::Chest;
        }
        else if (workingName == "neck") {
            return HumanoidBoneType::Neck;
        }
        else if (workingName == "head") {
            return HumanoidBoneType::Head;
        }

        const bool leftSide = workingName.contains("left") ||
            workingName.starts_with("l_") ||
            workingName.ends_with("_l") ||
            workingName.contains(".l") ||
            workingName.contains("l.") ||
            workingName.contains("_l_");

        if (HumanoidBoneType type = TryExtractSidedHumanoidBone(workingName, leftSide); type != HumanoidBoneType::Unknown) {
            return type;
        }

        SR_WARN("ExtractHumanoidBoneType() : failed to extract humanoid bone type from \"{}\" ({}) name!", name, workingName);
        return HumanoidBoneType::Unknown;
    }
}