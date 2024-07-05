//
// Created by Monika on 07.01.2023.
//

#include <Graphics/Animations/AnimationKey.h>
#include <Graphics/Animations/AnimationData.h>
#include <Graphics/Animations/AnimationGraph.h>

#include <Utils/ECS/GameObject.h>
#include <Utils/ECS/Transform.h>

namespace SR_ANIMATIONS_NS {
    void UnionAnimationKey::Update(const float_t progress, const UnionAnimationKey& prevKey, AnimationGameObjectData& animation) const noexcept {
        switch (type) {
            case AnimationKeyType::Translation:
                if (prevKey.data.translation.translation.IsEquals(data.translation.translation, SR_BIG_EPSILON)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.translation = prevKey.data.translation.translation.Lerp(data.translation.translation, progress);
                break;
            case AnimationKeyType::Rotation:
                if (prevKey.data.rotation.rotation.IsEquals(data.rotation.rotation, SR_BIG_EPSILON)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.rotation = prevKey.data.rotation.rotation.Slerp(data.rotation.rotation, progress);
                break;
            case AnimationKeyType::Scaling:
                if (prevKey.data.scaling.scaling.IsEquals(data.scaling.scaling, SR_BIG_EPSILON)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.scaling = prevKey.data.scaling.scaling.Lerp(data.scaling.scaling, progress);
                break;
            default:
                SRHalt("Unknown key type!");
        }

        animation.dirty = true;
    }

    void UnionAnimationKey::Set(AnimationGameObjectData& animation) const noexcept {
        switch (type) {
            case AnimationKeyType::Translation:
                if (animation.translation.has_value() && animation.translation.value().IsEquals(data.translation.translation, SR_BIG_EPSILON)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.translation = data.translation.translation;
                break;
            case AnimationKeyType::Rotation:
                if (animation.rotation.has_value() && animation.rotation.value().IsEquals(data.rotation.rotation, SR_BIG_EPSILON)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.rotation = data.rotation.rotation;
                break;
            case AnimationKeyType::Scaling:
                if (animation.scaling.has_value() && animation.scaling.value().IsEquals(data.scaling.scaling, SR_BIG_EPSILON)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.scaling = data.scaling.scaling;
                break;
            default:
                SRHalt("Unknown key type!");
        }

        animation.dirty = true;
    }

    void UnionAnimationKey::CopyFrom(const UnionAnimationKey& other) {
        time = other.time;
        type = other.type;

        switch (type) {
            case AnimationKeyType::Translation:
                data.translation = other.data.translation;
            break;
            case AnimationKeyType::Rotation:
                data.rotation = other.data.rotation;
            break;
            case AnimationKeyType::Scaling:
                data.scaling = other.data.scaling;
            break;
            default:
                SRHalt("Unknown key type!");
        }
    }
}
