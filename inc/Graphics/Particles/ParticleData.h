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

        SR_MATH_NS::FVector4 color;

        float lifetime;
        float maxLifetime;

        float size;
        float startSize;

        SR_MATH_NS::FVector3 rotation;
        SR_MATH_NS::FVector3 rotationSpeed;

        SR_MATH_NS::FVector3 direction;
    };
}

#endif //SRENGINE_PARTICLEDATA_H
