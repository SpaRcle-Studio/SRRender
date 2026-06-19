//
// Created by Monika on 14.06.2026.
//

#ifndef SR_ENGINE_GRAPHICS_HUMANOID_BONE_TYPE_H
#define SR_ENGINE_GRAPHICS_HUMANOID_BONE_TYPE_H

#include <Graphics/stdInclude.h>

#include <Utils/Common/Enumerations.h>

namespace SR_ANIMATIONS_NS {
    SR_ENUM_NS_CLASS_T(HumanoidBoneType, uint16_t,
        Unknown,

        // ================= BODY CORE =================
        Hips,

        Spine,
        Chest,
        UpperChest,

        Neck,
        Head,

        // ================= ARMS (LEFT) =================
        LeftShoulder,
        LeftUpperArm,
        LeftLowerArm,
        LeftHand,

        // ================= ARMS (RIGHT) =================
        RightShoulder,
        RightUpperArm,
        RightLowerArm,
        RightHand,

        // ================= LEGS (LEFT) =================
        LeftUpperLeg,
        LeftLowerLeg,
        LeftFoot,
        LeftToes,

        // ================= LEGS (RIGHT) =================
        RightUpperLeg,
        RightLowerLeg,
        RightFoot,
        RightToes,

        // ================= FINGERS (LEFT HAND) =================
        LeftThumbProximal,
        LeftThumbIntermediate,
        LeftThumbDistal,

        LeftIndexProximal,
        LeftIndexIntermediate,
        LeftIndexDistal,

        LeftMiddleProximal,
        LeftMiddleIntermediate,
        LeftMiddleDistal,

        LeftRingProximal,
        LeftRingIntermediate,
        LeftRingDistal,

        LeftLittleProximal,
        LeftLittleIntermediate,
        LeftLittleDistal,

        // ================= FINGERS (RIGHT HAND) =================
        RightThumbProximal,
        RightThumbIntermediate,
        RightThumbDistal,

        RightIndexProximal,
        RightIndexIntermediate,
        RightIndexDistal,

        RightMiddleProximal,
        RightMiddleIntermediate,
        RightMiddleDistal,

        RightRingProximal,
        RightRingIntermediate,
        RightRingDistal,

        RightLittleProximal,
        RightLittleIntermediate,
        RightLittleDistal,

        // ================= FACE (SIMPLE FULL SET) =================
        Jaw,
        Chin,

        LeftEye,
        RightEye,

        LeftEyebrowInner,
        LeftEyebrowMiddle,
        LeftEyebrowOuter,

        RightEyebrowInner,
        RightEyebrowMiddle,
        RightEyebrowOuter,

        Nose,

        MouthUpperLip,
        MouthLowerLip,
        MouthCornerLeft,
        MouthCornerRight,

        TongueBase,
        TongueMid,
        TongueTip
    );

    extern const SR_UTILS_NS::Vector<HumanoidBoneType>& GetHumanoidSkeletonHierarchy();

    extern HumanoidBoneType ExtractHumanoidBoneType(SR_UTILS_NS::StringAtom name);
}

#endif //SR_ENGINE_GRAPHICS_HUMANOID_BONE_TYPE_H
