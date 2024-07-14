//
// Created by Monika on 07.01.2023.
//

#include <Graphics/Animations/AnimationKey.h>
#include <Graphics/Animations/AnimationData.h>
#include <Graphics/Animations/AnimationGraph.h>

#include <Utils/ECS/GameObject.h>
#include <Utils/ECS/Transform.h>

namespace SR_ANIMATIONS_NS {
    void UnionAnimationKey::Update(const float_t progress, const UnionAnimationKey& prevKey, AnimationGameObjectData& animation, float_t tolerance) const noexcept {
        if (tolerance == 0.0f) SR_UNLIKELY_ATTRIBUTE {
            Update(progress, prevKey, animation);
            return;
        }

        switch (type) {
            case AnimationKeyType::Rotation:
                if (prevKey.data.rotation.rotation.IsEquals(data.rotation.rotation, tolerance)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.rotation = prevKey.data.rotation.rotation.Slerp(data.rotation.rotation, progress);
                break;
            case AnimationKeyType::Translation:
                if (prevKey.data.translation.translation.IsEquals(data.translation.translation, tolerance)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.translation = prevKey.data.translation.translation.Lerp(data.translation.translation, progress);
                break;
            case AnimationKeyType::Scaling:
                if (prevKey.data.scaling.scaling.IsEquals(data.scaling.scaling, tolerance)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.scaling = prevKey.data.scaling.scaling.Lerp(data.scaling.scaling, progress);
                break;
            default:
                SRHalt("Unknown key type!");
        }

        animation.dirty = true;
    }

    void UnionAnimationKey::Set(AnimationGameObjectData& animation, float_t tolerance) const noexcept {
        if (tolerance == 0.0f) SR_UNLIKELY_ATTRIBUTE {
            Set(animation);
            return;
        }

        switch (type) {
            case AnimationKeyType::Rotation:
                if (animation.rotation.has_value() && animation.rotation.value().IsEquals(data.rotation.rotation, tolerance)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.rotation = data.rotation.rotation;
                break;
            case AnimationKeyType::Translation:
                if (animation.translation.has_value() && animation.translation.value().IsEquals(data.translation.translation, tolerance)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.translation = data.translation.translation;
                break;
            case AnimationKeyType::Scaling:
                if (animation.scaling.has_value() && animation.scaling.value().IsEquals(data.scaling.scaling, tolerance)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.scaling = data.scaling.scaling;
                break;
            default:
                SRHalt("Unknown key type!");
        }

        animation.dirty = true;
    }

    void UnionAnimationKey::UpdateWithWeight(float_t progress, const UnionAnimationKey& prevKey, AnimationGameObjectData& animation, float_t weight) const noexcept {
        switch (type) {
            case AnimationKeyType::Rotation:
                if (!animation.rotation.has_value()) SR_UNLIKELY_ATTRIBUTE {
                    animation.rotation = prevKey.data.rotation.rotation.Slerp(data.rotation.rotation, progress);
                    break;
                }
                animation.rotation = animation.rotation.value().Slerp(prevKey.data.rotation.rotation
                    .Slerp(data.rotation.rotation, progress), weight);
                break;
            case AnimationKeyType::Translation:
                if (!animation.translation.has_value()) SR_UNLIKELY_ATTRIBUTE {
                    animation.translation = prevKey.data.translation.translation.Lerp(data.translation.translation, progress);
                    break;
                }
                animation.translation = animation.translation.value().Lerp(prevKey.data.translation.translation
                    .Lerp(data.translation.translation, progress), weight);
                break;
            case AnimationKeyType::Scaling:
                if (!animation.scaling.has_value()) SR_UNLIKELY_ATTRIBUTE {
                    animation.scaling = prevKey.data.scaling.scaling.Lerp(data.scaling.scaling, progress);
                    break;
                }
                animation.scaling = animation.scaling.value().Lerp(prevKey.data.scaling.scaling
                    .Lerp(data.scaling.scaling, progress), weight);
                break;
            default:
                SRHalt("Unknown key type!");
        }

        animation.dirty = true;
    }

    void UnionAnimationKey::SetWithWeight(AnimationGameObjectData& animation, float_t weight) const noexcept {
        switch (type) {
            case AnimationKeyType::Rotation:
                if (!animation.rotation.has_value()) SR_UNLIKELY_ATTRIBUTE {
                    animation.rotation = data.rotation.rotation;
                    break;
                }
                animation.rotation = animation.rotation.value().Slerp(data.rotation.rotation, weight);
                break;
            case AnimationKeyType::Translation:
                if (!animation.translation.has_value()) SR_UNLIKELY_ATTRIBUTE {
                    animation.translation = data.translation.translation;
                    break;
                }
                animation.translation = animation.translation.value().Lerp(data.translation.translation, weight);
                break;
            case AnimationKeyType::Scaling:
                if (!animation.scaling.has_value()) SR_UNLIKELY_ATTRIBUTE {
                    animation.scaling = data.scaling.scaling;
                    break;
                }
                animation.scaling = animation.scaling.value().Lerp(data.scaling.scaling, weight);
                break;
            default:
                SRHalt("Unknown key type!");
        }
    }

    void UnionAnimationKey::Update(const float_t progress, const UnionAnimationKey& prevKey, AnimationGameObjectData& animation) const noexcept {
        switch (type) {
            case AnimationKeyType::Rotation:
                animation.rotation = prevKey.data.rotation.rotation.Slerp(data.rotation.rotation, progress);
                break;
            case AnimationKeyType::Translation:
                animation.translation = prevKey.data.translation.translation.Lerp(data.translation.translation, progress);
                break;
            case AnimationKeyType::Scaling:
                animation.scaling = prevKey.data.scaling.scaling.Lerp(data.scaling.scaling, progress);
                break;
            default:
                SRHalt("Unknown key type!");
        }

        animation.dirty = true;
    }

    void UnionAnimationKey::Set(AnimationGameObjectData& animation) const noexcept {
        switch (type) {
            case AnimationKeyType::Rotation:
                animation.rotation = data.rotation.rotation;
                break;
            case AnimationKeyType::Translation:
                animation.translation = data.translation.translation;
                break;
            case AnimationKeyType::Scaling:
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
