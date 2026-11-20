//
// Created by Monika on 07.01.2023.
//

#ifndef SR_ENGINE_ANIMATIONKEY_H
#define SR_ENGINE_ANIMATIONKEY_H

#include <Graphics/Animations/Interpolation.h>
#include <Graphics/Animations/AnimationPose.h>
#include <Graphics/Animations/AnimationContext.h>

namespace SR_UTILS_NS {
    class GameObject;
}

namespace SR_ANIMATIONS_NS {
    class AnimationChannel;
    enum class AnimationChannelType : uint8_t;

    /// Задача ключа обеспечить необходимый переход из предыдущего ключа в этот в зависимости от интервала времени.
    /// Интервал времени задается от 0.f до 1.f в зависимости от положения перехода в момент времени.
    /// Переход должен работать и в обратную сторону (от 1.f до 0.f)

    struct UnionAnimationKey final {
        UnionAnimationKey() = default;
        ~UnionAnimationKey() = default;

        UnionAnimationKey(const UnionAnimationKey& other);
        UnionAnimationKey(UnionAnimationKey&& other) noexcept;

        UnionAnimationKey& operator=(const UnionAnimationKey& other);
        UnionAnimationKey& operator=(UnionAnimationKey&& other) noexcept;

        void SR_FASTCALL UpdateWithWeight(AnimationChannelType type, float_t progress, const UnionAnimationKey& prevKey, AnimationGameObjectData& animation, float_t weight) const noexcept;
        void SR_FASTCALL SetWithWeight(AnimationChannelType type, AnimationGameObjectData& animation, float_t weight) const noexcept;

        void SR_FASTCALL Update(AnimationChannelType type, float_t progress, const UnionAnimationKey& prevKey, AnimationGameObjectData& animation) const noexcept;
        void SR_FASTCALL Set(AnimationChannelType type, AnimationGameObjectData& animation) const noexcept;

        void SR_FASTCALL Update(AnimationChannelType type, float_t progress, const UnionAnimationKey& prevKey, AnimationGameObjectData& animation, float_t toreance) const noexcept;
        void SR_FASTCALL Set(AnimationChannelType type, AnimationGameObjectData& animation, float_t toreance) const noexcept;

        void CopyFrom(const UnionAnimationKey& other);
        void MoveFrom(UnionAnimationKey& other) noexcept;

    public:
        float_t time = 0.f;
        union Data {
            SR_MATH_NS::FVector3 translation;
            SR_MATH_NS::Quaternion rotation;
            SR_MATH_NS::FVector3 scaling;

            Data() { }
            ~Data() { }
        } data;

    };
}

#endif //SR_ENGINE_ANIMATIONKEY_H
