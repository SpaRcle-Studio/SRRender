//
// Created by Monika on 11.04.2026.
//

#include <Graphics/Particles/ParticleEmitter.h>
#include <Graphics/Particles/ParticleData.h>

#include <Codegen/ParticleEmitter.generated.hpp>

#include "Graphics/Render/RenderScene.h"

namespace SR_GRAPH_NS{

    void ParticleEmitter::InitializeParticle(){
        m_particles.resize(m_maxParticles);
    }

    void ParticleEmitter::SpawnParticle(){
        InitializeParticle();

        if (m_aliveParticles >= m_maxParticles){
            return;
        }

        auto& particle = m_particles[m_aliveParticles];

        particle.position = SR_MATH_NS::FVector3(0.0f);
        particle.velocity = SR_MATH_NS::FVector3(0.0f, 1.0f, 0.0f);

        particle.lifetime = 5.0f;
        particle.maxLifetime = 5.0f;

        particle.size = 1.0f;

        particle.color = SR_MATH_NS::FVector4(1.0f);

        ++m_aliveParticles;
    }

    void ParticleEmitter::UpdateParticle(float_t dt){
        for (uint32_t i = 0; i < m_aliveParticles;){
            auto& particle = m_particles[i];

            particle.position += particle.velocity * dt;

            particle.lifetime -= dt;

            if (particle.lifetime <= 0.0f){
                KillParticle(i);
                continue;
            }
            ++i;
        }
    }

    void ParticleEmitter::KillParticle(uint32_t index){
        const uint32_t last = m_aliveParticles - 1;

        m_particles[index] = m_particles[last];

        --m_aliveParticles;
    }

    void ParticleEmitter::UpdateEmitter(float_t dt){
        m_spawnTimer += dt;

        const float_t spawnInterval = 1.0f / m_spawnRate;

        while (m_spawnTimer >= spawnInterval){
            SpawnParticle();
            m_spawnTimer -= spawnInterval;
        }

        SR_INFO("Alive particles: {}", m_aliveParticles);

        UpdateParticle(dt);
    }

    void ParticleEmitter::OnEnable(){
        Super::OnEnable();

        if (auto* pRenderScene = TryGetRenderScene()){
            pRenderScene->GetParticleUpdater().RegisterEmitter(this);
        }
    }

    void ParticleEmitter::OnDisable() {
        Super::OnDisable();

        if (auto* pRenderScene = TryGetRenderScene()) {
            pRenderScene->GetParticleUpdater().RemoveEmitter(this);
        }
    }
}
