//
// Created by Monika on 25.04.2023.
//

#ifndef SR_ENGINE_GRAPHICS_ANIMATION_POSE_H
#define SR_ENGINE_GRAPHICS_ANIMATION_POSE_H

#include <Graphics/Animations/AnimationCommon.h>

#include <Utils/ECS/GameObject.h>

namespace SR_ANIMATIONS_NS {
    class Skeleton;
    class AnimationGameObjectData;
    class AnimationClip;

    class AnimationPose : public SR_HTYPES_NS::SharedPtr<AnimationPose>, public SR_UTILS_NS::SRClass {
        SR_CLASS()
        using Index = uint32_t;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<AnimationPose>;

    public:
        AnimationPose();
        ~AnimationPose() override;

    public:
        void SetGameObjectsCount(uint32_t count);

        void BlendLinear(AnimationPose& first, AnimationPose& second, float_t factor, QuaternionBlendMode quatBlendMode);
        void BlendAdditive(AnimationPose& base, AnimationPose& additive, float_t factor);

        SR_NODISCARD AnimationGameObjectData& GetGameObjectData(Index index) noexcept;
        SR_NODISCARD SR_UTILS_NS::Vector<AnimationGameObjectData>& GetGameObjects() noexcept { return m_gameObjects; }

    private:
        SR_UTILS_NS::Vector<AnimationGameObjectData> m_gameObjects;

    };
}

#endif //SR_ENGINE_GRAPHICS_ANIMATION_POSE_H
