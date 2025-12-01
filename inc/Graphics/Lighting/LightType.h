//
// Created by Monika on 13.09.2025.
//

#ifndef SR_ENGINE_RENDER_LIGHT_TYPE_H
#define SR_ENGINE_RENDER_LIGHT_TYPE_H

#include <Graphics/stdInclude.h>

#include <Utils/Common/Enumerations.h>

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(LightType, uint8_t,
        Directional, Point, Spot, Area, Probe
    )

    SR_ENUM_NS_CLASS_T(ShadowType, uint8_t,
        Soft, Hard
    )
}

#endif //SR_ENGINE_RENDER_LIGHT_TYPE_H
