//
// Created by Monika on 18.06.2026.
//

#ifndef SR_ENGINE_GRAPHICS_RETARGET_ANIMATION_H
#define SR_ENGINE_GRAPHICS_RETARGET_ANIMATION_H

#include <Graphics/Animations/AnimationChannel.h>

#include <Utils/Common/Singleton.h>

namespace SR_ANIMATIONS_NS {
    class SkeletonRig;

    class RetargetAnimation : public SR_UTILS_NS::Singleton<RetargetAnimation> {
        SR_REGISTER_SINGLETON(RetargetAnimation);
        using Channels = SR_UTILS_NS::Vector<AnimationChannel>;
    public:
        SR_NODISCARD bool Retarget(
            const SkeletonRig& sourceRig,
            const SkeletonRig& targetRig,
            const Channels& sourceChannels,
            Channels& outTargetChannels
        );
    };
}

#endif //SR_ENGINE_GRAPHICS_RETARGET_ANIMATION_H
