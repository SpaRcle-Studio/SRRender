//
// Created by Monika on 11.04.2026.
//

#include <Graphics/Particles/ParticleEmitter.h>

#include <Codegen/ParticleEmitter.generated.hpp>

namespace SR_GRAPH_NS{

    void ParticleEmitter::Initialize(){
        m_particleData.positions.resize(m_maxParticles);
        m_particleData.velocities.resize(m_maxParticles);

        m_particleData.lifetimes.resize(m_maxParticles);
        m_particleData.maxLifetimes.resize(m_maxParticles);

        m_particleData.sizes.resize(m_maxParticles);

        m_particles.colors.resize(m_maxParticles);

        m_particles.maxParticles = m_maxParticles;
    }
}