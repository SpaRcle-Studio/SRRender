//
// Created by Nariman on 08.07.2026.
//

#ifndef SRENGINE_PARTICLEEMISSIONMODULE_H
#define SRENGINE_PARTICLEEMISSIONMODULE_H
#include <Graphics/macros.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>

#include <Utils/ECS/Component.h>
#include <Utils/FileSystem/Path.h>
#include <Utils/Platform/PlatformType.h>


namespace SR_GRAPH_NS{
    class ParticleEmissionModule : SR_UTILS_NS::Serializable{
    public:
        ///@property
        float duration = 5.0f;
        bool looping = true;

        float rateOverTime = 10.0f;
    };
}

#endif //SRENGINE_PARTICLEEMISSIONMODULE_H
