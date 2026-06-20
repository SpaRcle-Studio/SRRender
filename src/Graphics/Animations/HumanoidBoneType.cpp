//
// Created by Monika on 15.06.2026.
//

#include <Graphics/Animations/HumanoidBoneType.h>

namespace SR_ANIMATIONS_NS {
    static int32_t ExtractFingerSegmentIndex(const SR_UTILS_NS::String& name) {
        /// Common patterns:
        /// - Thumb1/Thumb2/Thumb3
        /// - thumb_01/thumb_02/thumb_03
        /// - thumb.prox / thumb.inter / thumb.dist
        /// - thumb_metacarpal (treat as proximal)
        if (name.contains("dist") || name.contains("tip") || name.contains("03") || name.ends_with("3")) {
            return 3;
        }
        if (name.contains("inter") || name.contains("mid") || name.contains("02") || name.ends_with("2")) {
            return 2;
        }
        if (name.contains("prox") || name.contains("meta") || name.contains("01") || name.ends_with("1")) {
            return 1;
        }

        return 1;
    }

    HumanoidBoneType TryExtractSidedHumanoidBone(const SR_UTILS_NS::String& name, bool leftSide, const SR_HTYPES_NS::FlatHashSet<HumanoidBoneType>& mappedBones) {
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
            switch (ExtractFingerSegmentIndex(name)) {
                case 2:  return leftSide ? HumanoidBoneType::LeftThumbIntermediate : HumanoidBoneType::RightThumbIntermediate;
                case 3:  return leftSide ? HumanoidBoneType::LeftThumbDistal : HumanoidBoneType::RightThumbDistal;
                default: return leftSide ? HumanoidBoneType::LeftThumbProximal : HumanoidBoneType::RightThumbProximal;
            }
        }
        else if (name.contains("index")) {
            switch (ExtractFingerSegmentIndex(name)) {
                case 2:  return leftSide ? HumanoidBoneType::LeftIndexIntermediate : HumanoidBoneType::RightIndexIntermediate;
                case 3:  return leftSide ? HumanoidBoneType::LeftIndexDistal : HumanoidBoneType::RightIndexDistal;
                default: return leftSide ? HumanoidBoneType::LeftIndexProximal : HumanoidBoneType::RightIndexProximal;
            }
        }
        else if (name.contains("middle")) {
            switch (ExtractFingerSegmentIndex(name)) {
                case 2:  return leftSide ? HumanoidBoneType::LeftMiddleIntermediate : HumanoidBoneType::RightMiddleIntermediate;
                case 3:  return leftSide ? HumanoidBoneType::LeftMiddleDistal : HumanoidBoneType::RightMiddleDistal;
                default: return leftSide ? HumanoidBoneType::LeftMiddleProximal : HumanoidBoneType::RightMiddleProximal;
            }
        }
        else if (name.contains("ring")) {
            switch (ExtractFingerSegmentIndex(name)) {
                case 2:  return leftSide ? HumanoidBoneType::LeftRingIntermediate : HumanoidBoneType::RightRingIntermediate;
                case 3:  return leftSide ? HumanoidBoneType::LeftRingDistal : HumanoidBoneType::RightRingDistal;
                default: return leftSide ? HumanoidBoneType::LeftRingProximal : HumanoidBoneType::RightRingProximal;
            }
        }
        else if (name.contains("little") || name.contains("pinky")) {
            switch (ExtractFingerSegmentIndex(name)) {
                case 2:  return leftSide ? HumanoidBoneType::LeftLittleIntermediate : HumanoidBoneType::RightLittleIntermediate;
                case 3:  return leftSide ? HumanoidBoneType::LeftLittleDistal : HumanoidBoneType::RightLittleDistal;
                default: return leftSide ? HumanoidBoneType::LeftLittleProximal : HumanoidBoneType::RightLittleProximal;
            }
        }

        const bool upperArmMapped = mappedBones.contains(leftSide ? HumanoidBoneType::LeftUpperArm : HumanoidBoneType::RightUpperArm);
        const bool lowerArmMapped = mappedBones.contains(leftSide ? HumanoidBoneType::LeftLowerArm : HumanoidBoneType::RightLowerArm);

        const bool isArmPattern = name.contains("arm") && !name.contains("armature");

        /// Shoulder, UpperArm, LowerArm, Hand,
        if (name.contains("shoulder") || name.contains("clavicle")) {
            return leftSide ? HumanoidBoneType::LeftShoulder : HumanoidBoneType::RightShoulder;
        }
        else if (isArmPattern && !upperArmMapped) {
            return leftSide ? HumanoidBoneType::LeftUpperArm : HumanoidBoneType::RightUpperArm;
        }
        else if (isArmPattern && !lowerArmMapped) {
            return leftSide ? HumanoidBoneType::LeftLowerArm : HumanoidBoneType::RightLowerArm;
        }
        else if (name.contains("hand") || name.contains("wrist")) {
            return leftSide ? HumanoidBoneType::LeftHand : HumanoidBoneType::RightHand;
        }

        return HumanoidBoneType::Unknown;
    }

    const SR_UTILS_NS::Vector<HumanoidBoneType>& GetHumanoidSkeletonHierarchy() {
        static const SR_UTILS_NS::Vector<HumanoidBoneType> hierarchy = {
            HumanoidBoneType::Hips,
            HumanoidBoneType::Spine,
            HumanoidBoneType::Chest,
            HumanoidBoneType::UpperChest,
            HumanoidBoneType::Neck,
            HumanoidBoneType::Head,

            HumanoidBoneType::LeftShoulder,
            HumanoidBoneType::LeftUpperArm,
            HumanoidBoneType::LeftLowerArm,
            HumanoidBoneType::LeftHand,

            HumanoidBoneType::RightShoulder,
            HumanoidBoneType::RightUpperArm,
            HumanoidBoneType::RightLowerArm,
            HumanoidBoneType::RightHand,

            HumanoidBoneType::LeftUpperLeg,
            HumanoidBoneType::LeftLowerLeg,
            HumanoidBoneType::LeftFoot,
            HumanoidBoneType::LeftToes,

            HumanoidBoneType::RightUpperLeg,
            HumanoidBoneType::RightLowerLeg,
            HumanoidBoneType::RightFoot,
            HumanoidBoneType::RightToes
        };

        return hierarchy;
    }

    HumanoidBoneType ExtractHumanoidBoneTypeImpl(SR_UTILS_NS::StringAtom name, const SR_HTYPES_NS::FlatHashSet<HumanoidBoneType>& mappedBones) {
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
        else if (workingName.contains("spine") || workingName.contains("chest")) {
            return HumanoidBoneType::Spine;
        }
        else if (workingName == "neck") {
            return HumanoidBoneType::Neck;
        }
        else if (workingName == "head") {
            return HumanoidBoneType::Head;
        }

        const bool leftSide = workingName.contains("left") ||
            workingName.starts_with("l ") ||
            workingName.starts_with("l_") ||
            workingName.starts_with("l-") ||
            workingName.ends_with("_l") ||
            workingName.contains(".l") ||
            workingName.contains("l.") ||
            workingName.contains("_l_");

        if (HumanoidBoneType type = TryExtractSidedHumanoidBone(workingName, leftSide, mappedBones); type != HumanoidBoneType::Unknown) {
            return type;
        }

        SR_WARN("ExtractHumanoidBoneType() : failed to extract humanoid bone type from \"{}\" ({}) name!", name, workingName);
        return HumanoidBoneType::Unknown;
    }

    HumanoidBoneType ExtractHumanoidBoneType(SR_UTILS_NS::StringAtom name, SR_HTYPES_NS::FlatHashSet<HumanoidBoneType>& mappedBones) {
        SR_TRACY_ZONE;

        const HumanoidBoneType type = ExtractHumanoidBoneTypeImpl(name, mappedBones);
        if (type != HumanoidBoneType::Unknown) {
            mappedBones.insert(type);
        }
        return type;
    }
}