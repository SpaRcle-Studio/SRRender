//
// Created by Monika on 02.12.2025.
//

#ifndef SR_ENGINE_GRAPHICS_SSBO_USAGE_H
#define SR_ENGINE_GRAPHICS_SSBO_USAGE_H

#include <Graphics/macros.h>

#include <Utils/Common/Enumerations.h>

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(SSBOUsage, uint8_t,
        Unknown,
        GPUOnly, CPUOnly,
        CPUToGPU, GPUToCPU,
        CPUCopy,
        GPULazyAlloc,
        Auto, AutoPreferDevice, AutoPreferHost
    );
}

#endif //SR_ENGINE_GRAPHICS_SSBO_USAGE_H
