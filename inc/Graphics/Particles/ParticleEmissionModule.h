//
// Created by Nariman on 08.07.2026.
//

#ifndef SRENGINE_PARTICLEEMISSIONMODULE_H
#define SRENGINE_PARTICLEEMISSIONMODULE_H
#include <Graphics/stdInclude.h>
#include <Utils/Types/FastMemoryArray.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>

#include <Utils/ECS/Component.h>
#include <Utils/FileSystem/Path.h>
#include <Utils/Platform/PlatformType.h>


namespace SR_GRAPH_NS{

    SR_ENUM_NS_CLASS_T(EmissionType, uint8_t, OverTime, Burst)

    struct ParticleBurst {
        float time = 4.0f;
        uint32_t count = 50;
        bool emitted = false;
    };

    class ParticleEmissionModule : public SR_UTILS_NS::Serializable {
    SR_CLASS()
    public:

        ///@property
        float duration = 5.0f;
        ///@property
        bool looping = true;

        /// @property
        EmissionType mode = EmissionType::OverTime;
        ///@property @condition(This.mode == EmissionType::OverTime)
        float rateOverTime = 1.0f;

        ///@property @condition(This.mode == EmissionType::Burst)
        float timeInterval = 4.0f;
        ///@property @condition(This.mode == EmissionType::Burst)
        uint32_t burstCount = 50;


        SR_HTYPES_NS::FastMemoryArray<ParticleBurst> bursts;

        void CreateBurst(float time, uint32_t count);
    };
}

#endif //SRENGINE_PARTICLEEMISSIONMODULE_H
