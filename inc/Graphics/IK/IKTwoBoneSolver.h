//
// Created by Monika on 28.11.2025.
//

#ifndef SR_ENGINE_GRAPHICS_IK_TWO_BONE_SOLVER_H
#define SR_ENGINE_GRAPHICS_IK_TWO_BONE_SOLVER_H

#include <Graphics/stdInclude.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector3.h>

namespace SR_UTILS_NS {
    class Transform;
}

namespace SR_GRAPH_NS::IK {
    struct IKTwoBoneState {
        // Переменные для стабильности
        SR_MATH_NS::Quaternion lastRootRotation;
        SR_MATH_NS::Quaternion lastMidRotation;
        float upperArmLength = 0.f;
        float lowerArmLength = 0.f;
        float armLength = 0.f;
        bool initialized = false;

        // Исходные локальные направления для стабильности
        SR_MATH_NS::FVector3 rootToMidLocal;
        SR_MATH_NS::FVector3 midToTipLocal;
        SR_MATH_NS::Quaternion rootInitialRotation;
        SR_MATH_NS::Quaternion midInitialRotation;

        // Для предотвращения перекручивания
        SR_MATH_NS::FVector3 lastBendNormal;
        bool hasLastBendNormal = false;
    };

    struct IKTwoBoneParams {
        float weight = 1.f;
        float smoothing = 10.f;
        bool useInitialRotations = true;
        float rootAngleLimit = 0.f;
        float midAngleLimit = 0.f;
        float maxTwistChangePerFrame = 45.f;
        bool preventTwist = true;
        bool showDebugGizmos = true;
        float_t dt = 0.f;
    };

    void SolveTwoBone(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_UTILS_NS::Transform& target,
        const SR_UTILS_NS::Transform* pHint,
        IKTwoBoneState& state,
        const IKTwoBoneParams& params
    );
}

#endif //SR_ENGINE_GRAPHICS_IK_TWO_BONE_SOLVER_H
