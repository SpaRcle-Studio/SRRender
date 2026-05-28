//
// Created by Monika on 11.04.2026.
//

#include <Graphics/macros.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>

#include <Graphics/Particles/ParticleEmitter.h>
#include <Graphics/Particles/ParticleData.h>

#include <Codegen/ParticleEmitter.generated.hpp>

#include "Graphics/Render/RenderScene.h"
#include <Graphics/Pipeline/Pipeline.h>

namespace SR_GRAPH_NS{

    void ParticleEmitter::InitializeParticle(){
        m_particles.resize(m_maxParticles);

        m_instanceVertexBuffer.SetLayout(GetShaderVertexLayoutDescription());
    }

    void ParticleEmitter::Draw() {
        SR_INFO("DRAW PARTICLES");

        Calculate();

        GetPipeline()->SetDrawInstancesCount(m_aliveParticles);

        DrawRenderObject(
                this,
                6,
                m_virtualUBO,
                m_virtualDescriptor,
                m_dirtyMaterial,
                m_hasErrors
        );

        GetPipeline()->ResetDrawInstancesCount();
    }

    bool ParticleEmitter::ExecuteInEditMode() const {
        return true;
    }

    void ParticleEmitter::BuildInstanceData() {
        m_instanceData.resize(m_particles.size());

        for (uint32_t i = 0; i < m_aliveParticles; ++i){
            m_instanceData[i].position = m_particles[i].position;
            m_instanceData[i].size = 1.0f;
            m_instanceData[i].color = SR_MATH_NS::FVector4(1.0f);
        }
    }

    void ParticleEmitter::BuildInstanceVertexBuffer() {
        //SetVertexLayoutDescription(GetShaderVertexLayoutDescription());
        //m_instanceVertexBuffer.SetLayout(GetShaderVertexLayoutDescription());

        BuildInstanceData();
        m_instanceVertexBuffer.Allocate(m_aliveParticles);

        for (uint32_t i = 0; i < m_aliveParticles; ++i){
            m_instanceVertexBuffer.SetVertexT(i, SR_UTILS_NS::VertexAttribute::Position1,
                                              m_instanceData[i].position);
            m_instanceVertexBuffer.SetVertexT(i, SR_UTILS_NS::VertexAttribute::Color0,
                                              m_instanceData[i].color);
            m_instanceVertexBuffer.SetVertexT(i, SR_UTILS_NS::VertexAttribute::Custom0,
                                              m_instanceData[i].size);
        }

        auto* pPipeline = GetPipeline();

        if (!pPipeline) {
            SR_ERROR("Pipeline is null!");
            return;
        }

        m_VBO = GetPipeline()->AllocateVBO(m_VBO,
                                           m_aliveParticles * m_instanceVertexBuffer.GetLayout().GetStride(),
                                           m_instanceVertexBuffer.GetRawData());
    }

    void ParticleEmitter::SpawnParticle(){
        if (m_aliveParticles >= m_maxParticles){
            return;
        }

        auto& particle = m_particles[m_aliveParticles];

        particle.position = SR_MATH_NS::FVector3(0.0f);
        particle.velocity = SR_MATH_NS::FVector3(0.0f, 1.0f, 0.0f);

        particle.lifetime = 5.0f;
        particle.maxLifetime = 5.0f;

       // particle.size = 1.0f;

        //particle.color = SR_MATH_NS::FVector4(1.0f);

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

        UpdateParticle(dt);

        m_isVBODirty = true;
    }

    void ParticleEmitter::OnEnable(){
        InitializeParticle();

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

    const SR_UTILS_NS::VertexLayoutDescription&
    ParticleEmitter::GetShaderVertexLayoutDescription() const noexcept {
        static const auto description = SR_UTILS_NS::VertexLayoutDescription()
                .AddAttribute(SR_UTILS_NS::VertexAttribute::Position1, SR_UTILS_NS::VertexAttributeFormat::Float32, 3)
                .AddAttribute(SR_UTILS_NS::VertexAttribute::Color0, SR_UTILS_NS::VertexAttributeFormat::Float32, 4)
                .AddAttribute(SR_UTILS_NS::VertexAttribute::Custom0, SR_UTILS_NS::VertexAttributeFormat::Float32, 1)
                .SetInstanced(true);
        return description;
    }

    std::optional<int32_t> ParticleEmitter::GetVBO() const {
        const_cast<ParticleEmitter&>(*this).Calculate();
        if (m_VBO == SR_ID_INVALID){
            return std::nullopt;
        }

        return m_VBO;
    }

    bool ParticleEmitter::Bind() {
        if (m_VBO == SR_ID_INVALID){
            return false;
        }

        GetPipeline()->BindVBO(m_VBO);

        return true;
    }

    void ParticleEmitter::FreeVideoMemory() {
        Super::FreeVideoMemory();

        if (m_VBO != SR_ID_INVALID){
            GetPipeline()->FreeVBO(&m_VBO);
        }
    }

    void ParticleEmitter::Calculate() {
        if (!m_isVBODirty) {
            return;
        }


        if(m_aliveParticles > 0) {
            BuildInstanceVertexBuffer();
        }

        m_isVBODirty = false;
    }
}
