//
// Created by Monika on 09.02.2026.
//

#ifndef SR_ENGINE_RENDER_QUALITY_H
#define SR_ENGINE_RENDER_QUALITY_H

#include <Graphics/stdInclude.h>

#include <Utils/Common/Enumerations.h>

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(Quality, uint8_t,
        None,
        Low,
        Medium,
        High,
        Ultra,
        Extreme
    );
}

#endif //SR_ENGINE_RENDER_QUALITY_H
