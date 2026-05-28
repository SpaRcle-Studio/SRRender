//
// Created by Nariman on 15.05.2026.
//

#ifndef SRENGINE_PARTICLEDATA_H
#define SRENGINE_PARTICLEDATA_H


#include <Graphics/macros.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>

namespace SR_GRAPH_NS {
    struct ParticleData{
        SR_MATH_NS::FVector3 position;
        SR_MATH_NS::FVector3 velocity;

        float lifetime;
        float maxLifetime;
    };
}

#endif //SRENGINE_PARTICLEDATA_H
