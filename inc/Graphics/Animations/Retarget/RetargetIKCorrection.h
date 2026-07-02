//
// Created by Monika on 01.07.2026.
//

#ifndef SR_ENGINE_GRAPHICS_ANIMATIONS_RETARGET_IK_CORRECTION_H
#define SR_ENGINE_GRAPHICS_ANIMATIONS_RETARGET_IK_CORRECTION_H

#include <Graphics/Animations/Retarget/RetargetProfile.h>

namespace SR_UTILS_NS {
    class Transform;
}

namespace SR_ANIMATIONS_NS {
    class RetargetIKCorrection final : public RetargetIKCorrectionBase {
        SR_CLASS()
        using Super = RetargetIKCorrectionBase;
    public:
        struct TwoBoneIKState {
            bool initialized = false;

            float upperLen = 0.f;
            float lowerLen = 0.f;
            float totalLen = 0.f;

            SR_MATH_NS::FVector3 rootToMidLocal = SR_MATH_NS::FVector3::Forward();
            SR_MATH_NS::FVector3 midToTipLocal = SR_MATH_NS::FVector3::Forward();

            SR_MATH_NS::Quaternion rootInitialWorld = SR_MATH_NS::Quaternion::Identity();
            SR_MATH_NS::Quaternion midInitialWorld  = SR_MATH_NS::Quaternion::Identity();

            SR_MATH_NS::Quaternion lastRootWorld = SR_MATH_NS::Quaternion::Identity();
            SR_MATH_NS::Quaternion lastMidWorld  = SR_MATH_NS::Quaternion::Identity();

            SR_MATH_NS::FVector3 lastBendNormal = SR_MATH_NS::FVector3::Up();
            bool hasLastBendNormal = false;

            SR_MATH_NS::Quaternion lastTipWorld = SR_MATH_NS::Quaternion::Identity();
            bool hasLastTipWorld = false;

            bool tipOffsetInitialized = false;
            SR_MATH_NS::Quaternion tipRotationOffset = SR_MATH_NS::Quaternion::Identity();
        };

        struct TwoBoneIKParams {
            float weight = 1.f;
            float smoothing = 12.f;
            bool preventTwist = true;
            float maxTwistChangePerFrame = 60.f;
            bool tipRotationFromTarget = true;
        };

        void ResetState() override;

        void Apply(const RetargetAnimationContext& context) const override;

    private:
        SR_MATH_NS::FVector3 CalculateBendNormal(
            const SR_MATH_NS::FVector3& rootPos,
            const SR_MATH_NS::FVector3& targetPos,
            const SR_MATH_NS::FVector3* pHintPos,
            TwoBoneIKState& state,
            const TwoBoneIKParams& params
        ) const;

        void SolveTwoBoneLocalTarget(
            SR_UTILS_NS::Transform& root,
            SR_UTILS_NS::Transform& mid,
            SR_UTILS_NS::Transform& tip,
            const SR_MATH_NS::FVector3& targetWorldPos,
            const SR_MATH_NS::Quaternion& targetWorldRot,
            const SR_MATH_NS::FVector3* pHintWorldPos,
            TwoBoneIKState& state,
            const TwoBoneIKParams& params,
            float dt
        )  const;

    private:
        /// @property @group(Offsets)
        float_t m_scaleFactor = 1.f;
        /// @property @group(Offsets)
        SR_MATH_NS::FVector3 m_IKLeftHandOffset = SR_MATH_NS::FVector3::Zero();
        /// @property @group(Offsets)
        SR_MATH_NS::FVector3 m_IKRightHandOffset = SR_MATH_NS::FVector3::Zero();
        /// @property @group(Offsets)
        SR_MATH_NS::FVector3 m_IKLeftFootOffset = SR_MATH_NS::FVector3::Zero();
        /// @property @group(Offsets)
        SR_MATH_NS::FVector3 m_IKRightFootOffset = SR_MATH_NS::FVector3::Zero();

        /// @property
        float m_smoothing = 10.f;
        /// @property
        float m_twoBoneWeight = 1.f;
        /// @property
        uint8_t m_twoBoneIterations = 32;
        /// @property
        float m_ccdWeight = 0.35f;
        /// @property
        uint8_t m_ccdIterations = 6;
        /// @property
        bool m_twoBoneIKEnabled = true;
        /// @property
        bool m_handTipRotationFromTarget = false;
        /// @property
        bool m_footTipRotationFromTarget = true;

    private:
        /// Runtime-only persistent IK state across samples.
        mutable SR_HTYPES_NS::FlatHashMap<SR_UTILS_NS::StringAtom, TwoBoneIKState> m_state;
    };
}

#endif // SR_ENGINE_GRAPHICS_ANIMATIONS_RETARGET_IK_CORRECTION_H

