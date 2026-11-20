//
// Created by Monika on 07.01.2023.
//

#include <Graphics/Animations/AnimationKey.h>
#include <Graphics/Animations/AnimationData.h>
#include <Graphics/Animations/AnimationChannel.h>

#include <Utils/ECS/Transform.h>

namespace SR_ANIMATIONS_NS {
    UnionAnimationKey::UnionAnimationKey(const UnionAnimationKey& other) {
        CopyFrom(other);
    }

    UnionAnimationKey::UnionAnimationKey(UnionAnimationKey&& other) noexcept {
        MoveFrom(other);
    }

    UnionAnimationKey& UnionAnimationKey::operator=(const UnionAnimationKey& other) {
        if (this != &other) {
            CopyFrom(other);
        }
        return *this;
    }

    UnionAnimationKey& UnionAnimationKey::operator=(UnionAnimationKey&& other) noexcept {
        if (this != &other) {
            MoveFrom(other);
        }
        return *this;
    }

    void UnionAnimationKey::Update(AnimationChannelType type, const float_t progress, const UnionAnimationKey& prevKey, AnimationGameObjectData& animation, float_t tolerance) const noexcept {
        if (tolerance == 0.0f) SR_UNLIKELY_ATTRIBUTE {
            Update(type, progress, prevKey, animation);
            return;
        }

        switch (type) {
            case AnimationChannelType::Rotation:
                if (prevKey.data.rotation.IsEquals(data.rotation, tolerance)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.rotation = prevKey.data.rotation.Nlerp(data.rotation, progress);
                break;
            case AnimationChannelType::Translation:
                if (prevKey.data.translation.IsEquals(data.translation, tolerance)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.translation = prevKey.data.translation.Lerp(data.translation, progress);
                break;
            case AnimationChannelType::Scaling:
                if (prevKey.data.scaling.IsEquals(data.scaling, tolerance)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.scaling = prevKey.data.scaling.Lerp(data.scaling, progress);
                break;
            default:
                SRHalt("Unknown key type!");
        }

        animation.dirty = true;
    }

    void UnionAnimationKey::Set(AnimationChannelType type, AnimationGameObjectData& animation, float_t tolerance) const noexcept {
        if (tolerance == 0.0f) SR_UNLIKELY_ATTRIBUTE {
            Set(type, animation);
            return;
        }

        switch (type) {
            case AnimationChannelType::Rotation:
                if (animation.rotation.has_value() && animation.rotation.value().IsEquals(data.rotation, tolerance)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.rotation = data.rotation;
                break;
            case AnimationChannelType::Translation:
                if (animation.translation.has_value() && animation.translation.value().IsEquals(data.translation, tolerance)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.translation = data.translation;
                break;
            case AnimationChannelType::Scaling:
                if (animation.scaling.has_value() && animation.scaling.value().IsEquals(data.scaling, tolerance)) SR_UNLIKELY_ATTRIBUTE {
                    return;
                }
                animation.scaling = data.scaling;
                break;
            default:
                SRHalt("Unknown key type!");
        }

        animation.dirty = true;
    }

    void UnionAnimationKey::UpdateWithWeight(AnimationChannelType type, float_t progress, const UnionAnimationKey& prevKey, AnimationGameObjectData& animation, float_t weight) const noexcept {
        switch (type) {
            case AnimationChannelType::Rotation:
                if (!animation.rotation.has_value()) SR_UNLIKELY_ATTRIBUTE {
                    animation.rotation = prevKey.data.rotation.Nlerp(data.rotation, progress);
                    break;
                }
                animation.rotation = animation.rotation.value().Nlerp(prevKey.data.rotation
                    .Slerp(data.rotation, progress), weight);
                break;
            case AnimationChannelType::Translation:
                if (!animation.translation.has_value()) SR_UNLIKELY_ATTRIBUTE {
                    animation.translation = prevKey.data.translation.Lerp(data.translation, progress);
                    break;
                }
                animation.translation = animation.translation.value().Lerp(prevKey.data.translation
                    .Lerp(data.translation, progress), weight);
                break;
            case AnimationChannelType::Scaling:
                if (!animation.scaling.has_value()) SR_UNLIKELY_ATTRIBUTE {
                    animation.scaling = prevKey.data.scaling.Lerp(data.scaling, progress);
                    break;
                }
                animation.scaling = animation.scaling.value().Lerp(prevKey.data.scaling
                    .Lerp(data.scaling, progress), weight);
                break;
            default:
                SRHalt("Unknown key type!");
        }

        animation.dirty = true;
    }

    void UnionAnimationKey::SetWithWeight(AnimationChannelType type, AnimationGameObjectData& animation, float_t weight) const noexcept {
        switch (type) {
            case AnimationChannelType::Rotation:
                if (!animation.rotation.has_value()) SR_UNLIKELY_ATTRIBUTE {
                    animation.rotation = data.rotation;
                    break;
                }
                animation.rotation = animation.rotation.value().Nlerp(data.rotation, weight);
                break;
            case AnimationChannelType::Translation:
                if (!animation.translation.has_value()) SR_UNLIKELY_ATTRIBUTE {
                    animation.translation = data.translation;
                    break;
                }
                animation.translation = animation.translation.value().Lerp(data.translation, weight);
                break;
            case AnimationChannelType::Scaling:
                if (!animation.scaling.has_value()) SR_UNLIKELY_ATTRIBUTE {
                    animation.scaling = data.scaling;
                    break;
                }
                animation.scaling = animation.scaling.value().Lerp(data.scaling, weight);
                break;
            default:
                SRHalt("Unknown key type!");
        }
    }

    void UnionAnimationKey::Update(AnimationChannelType type, const float_t progress, const UnionAnimationKey& prevKey, AnimationGameObjectData& animation) const noexcept {
        switch (type) {
            case AnimationChannelType::Rotation:
                animation.rotation = prevKey.data.rotation.Nlerp(data.rotation, progress);
                break;
            case AnimationChannelType::Translation:
                animation.translation = prevKey.data.translation.Lerp(data.translation, progress);
                break;
            case AnimationChannelType::Scaling:
                animation.scaling = prevKey.data.scaling.Lerp(data.scaling, progress);
                break;
            default:
                SRHalt("Unknown key type!");
        }

        animation.dirty = true;
    }

    void UnionAnimationKey::Set(AnimationChannelType type, AnimationGameObjectData& animation) const noexcept {
        switch (type) {
            case AnimationChannelType::Rotation:
                animation.rotation = data.rotation;
                break;
            case AnimationChannelType::Translation:
                animation.translation = data.translation;
                break;
            case AnimationChannelType::Scaling:
                animation.scaling = data.scaling;
                break;
            default:
                SRHalt("Unknown key type!");
        }

        animation.dirty = true;
    }

    void UnionAnimationKey::CopyFrom(const UnionAnimationKey& other) {
        time = other.time;
        memcpy((void*)&data, (void*)&other.data, sizeof(Data));
    }

    void UnionAnimationKey::MoveFrom(UnionAnimationKey& other) noexcept {
        time = other.time;
        memmove((void*)&data, (void*)&other.data, sizeof(Data));
    }
}
