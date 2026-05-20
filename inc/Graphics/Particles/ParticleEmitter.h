//
// Created by Monika on 11.04.2026.
//

#ifndef SR_ENGINE_GRAPHICS_PARTICLE_EMITTER_H
#define SR_ENGINE_GRAPHICS_PARTICLE_EMITTER_H

#include <Graphics/Particles/ParticleData.h>
#include <Graphics/Types/IRenderComponent.h>

#include <Utils/ECS/Component.h>


namespace SR_GRAPH_NS {
    /// @category(Render.Particles)
    class ParticleEmitter : public SR_GTYPES_NS::IRenderComponent {
        SR_CLASS()
        using Super = SR_GTYPES_NS::IRenderComponent;
    public:
        SR_HTYPES_NS::FastMemoryArray<ParticleData> m_particles;
        uint32_t m_maxParticles = 1000;
        uint32_t m_aliveParticles = 0;
        float_t m_spawnRate = 10.0f;
        float_t m_spawnTimer = 0.0f;

        void InitializeParticle();
        void SpawnParticle();
        void UpdateParticle(float_t dt);
        void KillParticle(uint32_t index);
        void UpdateEmitter(float_t dt);
        void OnEnable() override;
        void OnDisable() override;
    };
}

#endif //SR_ENGINE_GRAPHICS_PARTICLE_EMITTER_H
