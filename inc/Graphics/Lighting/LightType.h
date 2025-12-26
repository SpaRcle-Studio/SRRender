//
// Created by Monika on 13.09.2025.
//

#ifndef SR_ENGINE_RENDER_LIGHT_TYPE_H
#define SR_ENGINE_RENDER_LIGHT_TYPE_H

#include <Graphics/stdInclude.h>

#include <Utils/Common/Enumerations.h>
#include <Utils/Math/Vector3.h>

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(LightType, uint8_t,
        Directional, Point, Spot, Area, Probe
    )

    SR_ENUM_NS_CLASS_T(ShadowType, uint8_t,
        Soft, Hard
    )

    struct DirectionalLightParams {
        SR_MATH_NS::FVector3 direction;
        SR_MATH_NS::FVector3 lightColor;
        SR_MATH_NS::FVector3 skyColor;
        SR_MATH_NS::FVector3 groundColor;
        float_t intensity = 1.f;
        float_t ambientIntensity = 1.f;
        float_t shadowStrength = 0.9f;
    };
}

#endif //SR_ENGINE_RENDER_LIGHT_TYPE_H
