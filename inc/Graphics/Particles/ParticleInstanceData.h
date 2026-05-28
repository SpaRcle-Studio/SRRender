//
// Created by Nariman on 21.05.2026.
//

#ifndef SRENGINE_PARTICLEINSTANCEDATA_H
#define SRENGINE_PARTICLEINSTANCEDATA_H

#include <Graphics/macros.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>

namespace SR_GRAPH_NS {
    struct ParticleInstanceData{
        SR_MATH_NS::FVector3 position;

        float_t size;

        SR_MATH_NS::FVector4 color;
    };
}

#endif //SRENGINE_PARTICLEINSTANCEDATA_H
