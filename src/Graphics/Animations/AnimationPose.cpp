//
// Created by Monika on 26.04.2023.
//

#include <Graphics/Animations/AnimationPose.h>
#include <Graphics/Animations/AnimationData.h>
#include <Graphics/Animations/AnimationChannel.h>

#include <Utils/ECS/Transform.h>

#include <Codegen/AnimationPose.generated.hpp>

namespace SR_ANIMATIONS_NS {
    AnimationPose::AnimationPose()
        : SR_HTYPES_NS::SharedPtr<AnimationPose>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    AnimationPose::~AnimationPose() = default;

    void AnimationPose::SetGameObjectsCount(uint32_t count) {
        m_gameObjects.resize(count);
    }

    AnimationGameObjectData& AnimationPose::GetGameObjectData(Index index) noexcept {
        if (index < m_gameObjects.size()) SR_LIKELY_ATTRIBUTE {
            return m_gameObjects[index];
        }
        static AnimationGameObjectData empty;
        SRHalt("Invalid index!");
        return empty;
    }

    void AnimationPose::BlendLinear(AnimationPose& first, AnimationPose& second, float_t factor, QuaternionBlendMode quatBlendMode) {
        SR_TRACY_ZONE;

        auto&& firstGameObjects = first.m_gameObjects;
        auto&& secondGameObjects = second.m_gameObjects;

        const uint32_t minSize = SR_MIN(firstGameObjects.size(), secondGameObjects.size());
        const uint32_t maxSize = SR_MAX(firstGameObjects.size(), secondGameObjects.size());

        m_gameObjects.resize(maxSize);

        for (size_t i = 0; i < minSize; ++i) {
            const AnimationGameObjectData& firstData = firstGameObjects[i];
            const AnimationGameObjectData& secondData = secondGameObjects[i];

            AnimationGameObjectData& resultData = m_gameObjects[i];

            resultData.dirty = firstData.dirty || secondGameObjects[i].dirty;
            if (!resultData.dirty) {
                continue;
            }

            if (firstData.translation.has_value() && secondGameObjects[i].translation.has_value()) {
                resultData.translation = SR_MATH_NS::FVector3::Lerp(firstData.translation.value(), secondData.translation.value(), factor);
            }
            else if (firstData.translation.has_value()) {
                resultData.translation = firstData.translation;
            }
            else if (secondData.translation.has_value()) {
                resultData.translation = secondData.translation;
            }
            else {
                resultData.translation.FastReset();
            }

            if (firstData.rotation.has_value() && secondData.rotation.has_value()) {
                if (quatBlendMode == QuaternionBlendMode::Nlerp) {
                    resultData.rotation = SR_MATH_NS::Quaternion::Nlerp(firstData.rotation.value(), secondData.rotation.value(), factor);
                }
                else {
                    resultData.rotation = SR_MATH_NS::Quaternion::Slerp(firstData.rotation.value(), secondData.rotation.value(), factor);
                }
            }
            else if (firstData.rotation.has_value()) {
                resultData.rotation = firstData.rotation;
            }
            else if (secondData.rotation.has_value()) {
                resultData.rotation = secondData.rotation;
            }
            else {
                resultData.rotation.FastReset();
            }

            if (firstData.scaling.has_value() && secondData.scaling.has_value()) {
                resultData.scaling = SR_MATH_NS::FVector3::Lerp(firstData.scaling.value(), secondData.scaling.value(), factor);
            }
            else if (firstData.scaling.has_value()) {
                resultData.scaling = firstData.scaling;
            }
            else if (secondData.scaling.has_value()) {
                resultData.scaling = secondData.scaling;
            }
            else {
                resultData.scaling.FastReset();
            }
        }

        for (size_t i = minSize; i < maxSize; ++i) {
            if (firstGameObjects.size() > secondGameObjects.size()) {
                m_gameObjects[i] = firstGameObjects[i];
            }
            else {
                m_gameObjects[i] = secondGameObjects[i];
            }
        }
    }

    void AnimationPose::BlendAdditive(AnimationPose& base, AnimationPose& additive, float_t factor) {
        SR_TRACY_ZONE;

        auto&& baseGameObjects = base.m_gameObjects;
        auto&& additiveGameObjects = additive.m_gameObjects;

        const uint32_t minSize = SR_MIN(baseGameObjects.size(), additiveGameObjects.size());
        const uint32_t maxSize = SR_MAX(baseGameObjects.size(), additiveGameObjects.size());

        m_gameObjects.resize(maxSize);

        for (size_t i = 0; i < minSize; ++i) {
            const AnimationGameObjectData& baseData = baseGameObjects[i];
            const AnimationGameObjectData& additiveData = additiveGameObjects[i];

            AnimationGameObjectData& resultData = m_gameObjects[i];

            resultData.dirty = baseData.dirty || additiveData.dirty;
            if (!resultData.dirty) {
                continue;
            }

            if (baseData.translation.has_value() && additiveData.translation.has_value()) {
                resultData.translation = baseData.translation.value() + additiveData.translation.value() * factor;
            }
            else if (baseData.translation.has_value()) {
                resultData.translation = baseData.translation;
            }
            else if (additiveData.translation.has_value()) {
                resultData.translation = additiveData.translation;
            }
            else {
                resultData.translation.FastReset();
            }

            if (baseData.rotation.has_value() && additiveData.rotation.has_value()) {
                resultData.rotation = baseData.rotation.value() * SR_MATH_NS::Quaternion::FromEulerAngles(additiveData.rotation.value().EulerAngle() * factor);
            }
            else if (baseData.rotation.has_value()) {
                resultData.rotation = baseData.rotation;
            }
            else if (additiveData.rotation.has_value()) {
                resultData.rotation = additiveData.rotation;
            }
            else {
                resultData.rotation.FastReset();
            }

            if (baseData.scaling.has_value() && additiveData.scaling.has_value()) {
                resultData.scaling = baseData.scaling.value() + additiveData.scaling.value() * factor;
            }
            else if (baseData.scaling.has_value()) {
                resultData.scaling = baseData.scaling;
            }
            else if (additiveData.scaling.has_value()) {
                resultData.scaling = additiveData.scaling;
            }
            else {
                resultData.scaling.FastReset();
            }
        }

        for (size_t i = minSize; i < maxSize; ++i) {
            if (baseGameObjects.size() > additiveGameObjects.size()) {
                m_gameObjects[i] = baseGameObjects[i];
            }
            else {
                m_gameObjects[i] = additiveGameObjects[i];
            }
        }
    }
}