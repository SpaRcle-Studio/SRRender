//
// Created by Monika on 23.11.2025.
//

#ifndef SR_ENGINE_GRAPHICS_IK_UTILS_H
#define SR_ENGINE_GRAPHICS_IK_UTILS_H

#include <Graphics/stdInclude.h>

#include <Utils/Math/Vector3.h>

namespace SR_UTILS_NS {
    class Transform;
}

namespace SR_GRAPH_NS::IK {
    // Структура для хранения данных о кости (аналог FLimbLink из UE)
    struct LimbLink {
        SR_MATH_NS::FVector3 Location;      // Позиция кости
        float_t Length;                      // Длина кости
        SR_MATH_NS::FVector3 RealBendDir;   // Кэшированное направление изгиба
        SR_MATH_NS::FVector3 BaseBendDir;    // Базовое направление изгиба
    };

    struct IKState {
        SR_MATH_NS::FVector3 previousBendAxis;

        std::optional<SR_MATH_NS::Quaternion> rootBaseRotation;
        SR_MATH_NS::Quaternion rootCurrentRotation;

        std::optional<SR_MATH_NS::Quaternion> midBaseRotation;
        SR_MATH_NS::Quaternion midCurrentRotation;
    };


    void SolveTwoBoneIK(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_UTILS_NS::Transform& target,
        const std::optional<SR_MATH_NS::FVector3>& hintPosition,
        IKState& ikState,
        float_t targetPosWeight,
        float_t targetRotWeight,
        float_t hintWeight
    );


    void SolveTwoBoneIK_Twist(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_UTILS_NS::Transform& target,
        const std::optional<SR_MATH_NS::FVector3>& hintPosition,
        IKState& ikState,
        float_t targetPosWeight,
        float_t targetRotWeight,
        float_t hintWeight
    );

    void SolveTwoBoneIK_WithFixes(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_UTILS_NS::Transform& target,
        const std::optional<SR_MATH_NS::FVector3>& hintPosition,
        IKState& ikState,
        float_t targetPosWeight,
        float_t targetRotWeight,
        float_t hintWeight
    );

    void SolveTwoBoneIK_GLM(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_UTILS_NS::Transform& target,
        const std::optional<SR_MATH_NS::FVector3>& hintPosition,
        IKState& ikState,
        float_t targetPosWeight,
        float_t targetRotWeight,
        float_t hintWeight
    );

    // Реализация IK из Unreal Engine
    void SolveTwoBoneIK_UE(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_MATH_NS::FVector3& targetLocation,
        const SR_MATH_NS::FVector3& hingeRotationAxis
    );
}

#endif //SR_ENGINE_GRAPHICS_IK_UTILS_H
