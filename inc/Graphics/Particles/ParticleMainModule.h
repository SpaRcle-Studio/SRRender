//
// Created by Nariman on 04.06.2026.
//

#ifndef SRENGINE_PARTICLEMAINMODULE_H
#define SRENGINE_PARTICLEMAINMODULE_H


#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>



namespace SR_GRAPH_NS {
    struct ParticleMainModule {
        float startMaxLifetime = 7.0f;
        float startMinLifetime = 4.0f;

        float startMaxSpeed = 7.0f;
        float startMinSpeed = 4.0f;
        float endSpeed = 0.0f;

        float startMinSize = 1.0f;
        float startMaxSize = 2.0f;
        float endSize = 0.0f;

        float gravity = -1.0;
        SR_MATH_NS::FVector4 m_startColor = SR_MATH_NS::FVector4(1.0f, 0.5f, 0.0f, 1.0f);
        SR_MATH_NS::FVector4 m_endColor = SR_MATH_NS::FVector4(0.2f, 0.0f, 1.0f, 1.0f);

        SR_MATH_NS::FVector3 startRotationSpeed = SR_MATH_NS::FVector3(90.0f, 0.0f, 180.0f);

    };
};
#endif //SRENGINE_PARTICLEMAINMODULE_H
