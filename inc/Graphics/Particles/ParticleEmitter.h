//
// Created by Monika on 11.04.2026.
//

#ifndef SR_ENGINE_GRAPHICS_PARTICLE_EMITTER_H
#define SR_ENGINE_GRAPHICS_PARTICLE_EMITTER_H

#include <Graphics/Particles/ParticleData.h>
#include <Graphics/Types/IRenderComponent.h>
#include <Graphics/Particles/ParticleInstanceData.h>
#include <Graphics/Particles/ParticleMainModule.h>
#include <Graphics/Particles/ParticleShapeModule.h>
#include <Graphics/Particles/ParticleRendererModule.h>


//include <Graphics/Common/Vertices.h>

#include <Utils/ECS/Component.h>



namespace SR_GRAPH_NS {
    /// @category(Render.Particles)
    class ParticleEmitter : public SR_GTYPES_NS::IRenderComponent {
        SR_CLASS()
        using Super = SR_GTYPES_NS::IRenderComponent;

    public:
        SR_HTYPES_NS::FastMemoryArray<ParticleData> m_particles;
        SR_HTYPES_NS::FastMemoryArray<ParticleInstanceData> m_instanceData;
        SR_UTILS_NS::VertexDataBuffer m_instanceVertexBuffer;


        uint32_t m_maxParticles = 1000;
        uint32_t m_aliveParticles = 0;
        float_t m_spawnRate = 10.0f;
        float_t m_spawnTimer = 0.0f;

        int32_t m_VBO = SR_ID_INVALID;

        int32_t m_virtualUBO = SR_ID_INVALID;
        int32_t m_virtualDescriptor = SR_ID_INVALID;

        void InitializeParticle();
        void BuildInstanceData();
        void BuildInstanceVertexBuffer();
        void SpawnParticle();
        void UpdateParticle(float_t dt);
        void UpdateEmitter(float_t dt);
        void KillParticle(uint32_t index);
        void OnEnable() override;
        void OnDisable() override;


        void FreeVideoMemory() override;
        SR_NODISCARD std::optional<int32_t> GetVBO() const override;
        bool Bind() override;
        const SR_UTILS_NS::VertexLayoutDescription& GetShaderVertexLayoutDescription() const noexcept override;

        void Draw() override;
        bool ExecuteInEditMode() const override;

        void UseMaterial(SR_GTYPES_NS::Shader& shader) override;
        void UseModelMatrix(SR_GTYPES_NS::Shader& shader) override;
        SR_NODISCARD int32_t GetVirtualUBO() const override { return m_virtualUBO; }

        //ShapeModule& GetShapeModule() noexcept {return m_shape;}

    private:
        void Calculate();
    private:
        bool m_isVBODirty = true;
        ParticleMainModule m_main;
        ParticleShape::Ptr m_shape;
        ParticleRenderMode m_renderer;

    };
}

#endif //SR_ENGINE_GRAPHICS_PARTICLE_EMITTER_H
