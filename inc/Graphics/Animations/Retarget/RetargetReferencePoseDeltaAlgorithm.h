//
// Created by Monika on 01.07.2026.
//

#ifndef SR_ENGINE_GRAPHICS_ANIMATIONS_RETARGET_REFERENCE_POSE_DELTA_ALGORITHM_H
#define SR_ENGINE_GRAPHICS_ANIMATIONS_RETARGET_REFERENCE_POSE_DELTA_ALGORITHM_H

#include <Graphics/Animations/Retarget/RetargetProfile.h>

namespace SR_ANIMATIONS_NS {
    struct RetargetReferencePoseDeltaAlgorithmState {
        struct RotationFollowState {
            bool initialized = false;
            bool hasLastLocal = false;
            SR_MATH_NS::Quaternion sourceToTargetOffset;
            SR_MATH_NS::Quaternion lastLocal;
        };
        using RotationFollowStateMap = std::map<SR_UTILS_NS::StringAtom, RotationFollowState>;

        struct RetargetFrameContext {
            explicit RetargetFrameContext(RotationFollowStateMap& rotationFollowStates)
                : rotationFollowStates(rotationFollowStates)
            { }
            SR_ANIMATIONS_NS::Skeleton::Ptr pSourceSkeleton;
            SR_ANIMATIONS_NS::Skeleton::Ptr pTargetSkeleton;
            RotationFollowStateMap& rotationFollowStates;
            SR_MATH_NS::FVector3 targetHipsOffset;
            SR_MATH_NS::FVector3 scaleFactor = SR_MATH_NS::FVector3::One();
        };

        SR_UTILS_NS::Vector<SR_UTILS_NS::GameObject::Ptr> gameObjects;
        SR_UTILS_NS::Vector<AnimationGameObjectData> poseGameObjects;
        SR_UTILS_NS::Vector<ChannelUpdateContext> channelContexts;
        SR_HTYPES_NS::FastMemoryArray<uint32_t> channelPlayState;
        RotationFollowStateMap rotationFollowStates;
        float_t animationTime = 0.f;
        bool initialized = false;
    };

    class RetargetReferencePoseDeltaAlgorithm final : public RetargetAlgorithmBase {
        SR_CLASS()
        using Super = RetargetAlgorithmBase;
    public:
        bool Retarget(RetargetAnimationContext& context) const override;

        static void RetargetFrame(RetargetReferencePoseDeltaAlgorithmState::RetargetFrameContext& context, uint32_t frame);

    private:
        void PrepareState(const RetargetAnimationContext& context) const;
        bool AnimateSourceObject(const RetargetAnimationContext& context, float_t dt) const;
        void ApplyAnimation(const RetargetAnimationContext& context) const;
        void SaveFrameToAnimation(RetargetAnimationContext& context, uint32_t frame) const;
        void ClearState() const;

    private:
        SR_NODISCARD static RetargetReferencePoseDeltaAlgorithmState& GetState();

    private:
        /// @property @group(Animation)
        float_t m_bakeFPS = 60.f;
        /// @property @group(Animation)
        float_t m_animationWeight = 1.f;
        /// @property @group(Animation)
        uint16_t m_animationFrameRate = 1;
        /// @property @group(Animation) @drag(0.01f)
        float_t m_animationTolerance = 1.0f;
        /// @property @group(Animation)
        bool m_animationFPSCompensation = false;

        /// @property @group(Offsets)
        float_t m_scaleFactor = 1.f;
        /// @property @group(Offsets)
        SR_MATH_NS::FVector3 m_targetHipsOffset = SR_MATH_NS::FVector3::Zero();

    };
}

#endif // SR_ENGINE_GRAPHICS_ANIMATIONS_RETARGET_REFERENCE_POSE_DELTA_ALGORITHM_H

