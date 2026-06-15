//
// Created by Monika on 25.04.2023.
//

#ifndef SR_ENGINE_ANIMATIONPOSE_H
#define SR_ENGINE_ANIMATIONPOSE_H

#include <Utils/ECS/GameObject.h>

#include <Graphics/Animations/AnimationCommon.h>

namespace SR_ANIMATIONS_NS {
    class Skeleton;
    class AnimationGameObjectData;
    class AnimationClip;

    class AnimationPose : public SR_UTILS_NS::NonCopyable {
        using Index = uint32_t;
    public:
        ~AnimationPose() override;

    public:
        void SetGameObjectsCount(uint32_t count);

        void BlendLinear(AnimationPose& first, AnimationPose& second, float_t factor, QuaternionBlendMode quatBlendMode);
        void BlendAdditive(AnimationPose& base, AnimationPose& additive, float_t factor);

        SR_NODISCARD AnimationGameObjectData& GetGameObjectData(Index index) noexcept;

        SR_NODISCARD std::vector<AnimationGameObjectData>& GetGameObjects() noexcept { return m_gameObjects; }

    private:
        std::vector<AnimationGameObjectData> m_gameObjects;

    };
}

#endif //SR_ENGINE_ANIMATIONPOSE_H
