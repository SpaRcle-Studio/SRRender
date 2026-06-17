//
// Created by Monika on 03.07.2024.
//

#ifndef SR_ENGINE_ANIMATION_CONTEXT_H
#define SR_ENGINE_ANIMATION_CONTEXT_H

#include <Graphics/Animations/AnimationData.h>

#include <Utils/Types/SortedVector.h>
#include <Utils/ECS/GameObject.h>

namespace SR_ANIMATIONS_NS {
    class AnimationPose;
    class AnimationGraph;
    class Skeleton;
    class SkeletonRig;

    struct UpdateContext {
        float_t dt = 0.f;
        float_t weight = 1.f;
        bool fpsCompensation = false;
        uint16_t frameRate = 1;
        AnimationPose* pPose = nullptr;
        AnimationGraph* pGraph = nullptr;
        const SkeletonRig* pRig = nullptr;
        float_t tolerance = 0.001f;
    };

    struct ChannelUpdateContext {
        std::optional<uint16_t> gameObjectIndex;
    };

    struct CompileContext {
        explicit CompileContext(std::vector<SR_UTILS_NS::GameObject::Ptr>& gameObjects)
            : gameObjects(gameObjects)
        { }

        Skeleton* pSkeleton = nullptr;
        const SkeletonRig* pRig = nullptr;

        std::vector<SR_UTILS_NS::GameObject::Ptr>& gameObjects;
    };
}

#endif //SR_ENGINE_ANIMATION_CONTEXT_H
