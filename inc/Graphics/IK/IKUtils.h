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
    void SolveTwoBoneIK(
        SR_UTILS_NS::Transform& root,
        SR_UTILS_NS::Transform& mid,
        SR_UTILS_NS::Transform& tip,
        const SR_UTILS_NS::Transform& target,
        const std::optional<SR_MATH_NS::FVector3>& hintPosition,
        float_t targetPosWeight,
        float_t targetRotWeight,
        float_t hintWeight
    );
}

#endif //SR_ENGINE_GRAPHICS_IK_UTILS_H
