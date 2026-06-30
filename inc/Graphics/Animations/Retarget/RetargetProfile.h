//
// Created by Monika on 20.06.2026.
//

#ifndef SR_ENGINE_GRAPHICS_ANIMATION_RETARGET_PROFILE_H
#define SR_ENGINE_GRAPHICS_ANIMATION_RETARGET_PROFILE_H

#include <Graphics/Animations/SkeletonRig.h>

namespace SR_ANIMATIONS_NS {
    /// @abstract
    class RetargetAlgorithmBase : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<RetargetAlgorithmBase> {
        SR_STRUCT()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<RetargetAlgorithmBase>;

    };

    /// @abstract
    class RetargetIKCorrectionBase : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<RetargetAlgorithmBase> {
        SR_STRUCT()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<RetargetIKCorrectionBase>;

    };

    /// @abstract
    class RetargetPoseRefinementBase : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<RetargetPoseRefinementBase> {
        SR_STRUCT()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<RetargetPoseRefinementBase>;

    };

    struct RetargetProfileEmbedded : public SR_UTILS_NS::Serializable {
        SR_STRUCT()
    public:
        /// @property @tooltip(Опционален если использовать напрямую в AnimationClip)
        SR_UTILS_NS::ResourceRef<SkeletonRig> sourceRig;
        /// @property
        SR_UTILS_NS::ResourceRef<SkeletonRig> targetRig;

        /// @property @notNull
        RetargetAlgorithmBase::Ptr algorithm;
        /// @property @notNull
        RetargetIKCorrectionBase::Ptr IKCorrection;
        /// @property @notNull
        RetargetPoseRefinementBase::Ptr poseRefinement;

    };

    /// @extension(retargetProfile)
    class RetargetProfile : public SR_UTILS_NS::Asset {
        SR_CLASS()
    public:

    private:
        /// @property @noHeader
        RetargetProfileEmbedded m_embedded;

    };

    class AnimationChannel;

    struct RetargetAnimationContext {
        using Channels = SR_UTILS_NS::Vector<AnimationChannel>;

        RetargetAnimationContext(const Channels& sourceChannels, const SkeletonRig& sourceRig, const SkeletonRig& targetRig)
            : sourceChannels(sourceChannels)
            , sourceRig(sourceRig)
            , targetRig(targetRig)
        { }

        const RetargetProfileEmbedded* pProfile = nullptr;
        const SkeletonRig& sourceRig;
        const SkeletonRig& targetRig;
        const Channels& sourceChannels;

        SR_UTILS_NS::Vector<AnimationChannel> targetChannels;
    };

    class RetargetAnimationSystem : public SR_UTILS_NS::Singleton<RetargetAnimationSystem> {
        SR_REGISTER_SINGLETON(RetargetAnimationSystem)
    public:
        bool Retarget(const RetargetAnimationContext& context);

    };
}

#endif //SR_ENGINE_GRAPHICS_ANIMATION_RETARGET_PROFILE_H
