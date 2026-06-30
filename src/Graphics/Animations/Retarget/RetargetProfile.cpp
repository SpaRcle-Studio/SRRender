//
// Created by Monika on 20.06.2026.
//

#include <Graphics/Animations/Retarget/RetargetProfile.h>
#include <Graphics/Animations/AnimationChannel.h>

#include <Codegen/RetargetProfile.generated.hpp>

namespace SR_ANIMATIONS_NS {
    bool RetargetAnimationSystem::Retarget(const RetargetAnimationContext& context) {
        if (context.sourceChannels.empty()) {
            SR_ERROR("RetargetAnimationSystem::Retarget() : source animation has no channels!");
            return false;
        }

        return false;
    }
}