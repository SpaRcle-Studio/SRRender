//
// Created by Monika on 20.06.2026.
//

#ifndef SR_ENGINE_GRAPHICS_ANIMATION_RETARGET_PROFILE_H
#define SR_ENGINE_GRAPHICS_ANIMATION_RETARGET_PROFILE_H

#include <Graphics/Animations/SkeletonRig.h>
#include <Graphics/Animations/AnimationChannel.h>
#include <Graphics/Animations/Skeleton.h>

namespace SR_ANIMATIONS_NS {
    struct RetargetAnimationContext;

    /// @abstract
    class RetargetAlgorithmBase : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<RetargetAlgorithmBase> {
        SR_STRUCT()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<RetargetAlgorithmBase>;

        RetargetAlgorithmBase()
            : Ptr(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        { }

        virtual bool Retarget(RetargetAnimationContext& context) const { return false; }

    };

    /// @abstract
    class RetargetIKCorrectionBase : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<RetargetIKCorrectionBase> {
        SR_STRUCT()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<RetargetIKCorrectionBase>;

        RetargetIKCorrectionBase()
            : Ptr(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        { }

        virtual void ResetState() { }
        virtual void Apply(const RetargetAnimationContext& context) const { }

    };

    /// @abstract
    class RetargetPoseRefinementBase : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<RetargetPoseRefinementBase> {
        SR_STRUCT()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<RetargetPoseRefinementBase>;

        RetargetPoseRefinementBase()
            : Ptr(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        { }

        virtual void Refine(const RetargetAnimationContext& context) const { }

    };

    struct RetargetProfileEmbedded : public SR_UTILS_NS::Serializable {
        SR_STRUCT()
    public:
        SR_NODISCARD static RetargetProfileEmbedded CreateDefault();

        bool operator==(const RetargetProfileEmbedded& other) const noexcept {
            return sourceRig == other.sourceRig &&
                targetRig == other.targetRig &&
                algorithm == other.algorithm &&
                IKCorrection == other.IKCorrection &&
                poseRefinement == other.poseRefinement;
        }

        bool operator!=(const RetargetProfileEmbedded& other) const noexcept {
            return !(*this == other);
        }

    public:
        /// @property @tooltip(Опционален если использовать напрямую в AnimationClip)
        SR_UTILS_NS::ResourceRef<SkeletonRig> sourceRig;
        /// @property
        SR_UTILS_NS::ResourceRef<SkeletonRig> targetRig;

        /// @property @notNull
        RetargetAlgorithmBase::Ptr algorithm;
        /// @property
        RetargetIKCorrectionBase::Ptr IKCorrection;
        /// @property
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

        SR_UTILS_NS::GameObject::Ptr pSourceSkeletonHierarchy;
        SR_UTILS_NS::GameObject::Ptr pTargetSkeletonHierarchy;

        SR_ANIMATIONS_NS::Skeleton::Ptr pSourceSkeleton;
        SR_ANIMATIONS_NS::Skeleton::Ptr pTargetSkeleton;

        uint32_t maxKeyFrame = 0;
        SR_UTILS_NS::Vector<AnimationChannel> targetChannels;
    };

    class RetargetAnimationSystem : public SR_UTILS_NS::Singleton<RetargetAnimationSystem> {
        SR_REGISTER_SINGLETON(RetargetAnimationSystem)
    public:
        bool Retarget(RetargetAnimationContext& context);

    private:
        bool PrepareContext(RetargetAnimationContext& context) const;

        SR_NODISCARD SR_UTILS_NS::GameObject::Ptr BuildSkeletonHierarchy(const SkeletonRig& rig) const;

    };
}

#endif //SR_ENGINE_GRAPHICS_ANIMATION_RETARGET_PROFILE_H
