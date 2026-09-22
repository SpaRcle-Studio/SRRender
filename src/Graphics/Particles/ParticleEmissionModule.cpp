//
// Created by Nariman on 21.09.2026.
//

#include <Graphics/Particles/ParticleEmissionModule.h>
#include <Codegen/ParticleEmissionModule.generated.hpp>

namespace SR_GRAPH_NS{
    void ParticleEmissionModule::CreateBurst(float time, uint32_t count) {
        ParticleBurst burst;
        burst.time = time;
        burst.count = count;
        bursts.emplace_back(burst);
    }
};