//
// Created by Monika on 11.04.2026.
//

#include <Graphics/macros.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>

#include <Graphics/Particles/ParticleEmitter.h>
#include <Graphics/Particles/ParticleData.h>

#include <Graphics/Render/RenderScene.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Material/BaseMaterial.h>

#include <Utils/Types/RawMesh.h>

#include <Codegen/ParticleEmitter.generated.hpp>

namespace SR_GRAPH_NS{

    void ParticleEmitter::InitializeParticle(){
        m_particles.resize(m_maxParticles);
        m_instanceData.resize(m_particles.size());

        m_instanceVertexBuffer.SetLayout(GetShaderVertexLayoutDescription());
        m_instanceVertexBuffer.Allocate(m_maxParticles);

        m_VBO = GetPipeline()->AllocateVBO(
                m_VBO,
                m_maxParticles * m_instanceVertexBuffer.GetLayout().GetStride(),
                m_instanceVertexBuffer.GetRawData());
    }

    void ParticleEmitter::Draw() {
        if(m_aliveParticles == 0) {
            return;
        }

        Calculate();

        GetPipeline()->SetDrawInstancesCount(m_aliveParticles);


        DrawRenderObject(
                    this,
                    IsValidMeshId() ? GetIndices().size() : 6,
                    m_virtualUBO,
                    m_virtualDescriptor,
                    m_dirtyMaterial,
                    m_hasErrors);

        GetPipeline()->ResetDrawInstancesCount();
    }

    bool ParticleEmitter::ExecuteInEditMode() const {
        return true;
    }

    void ParticleEmitter::BuildInstanceData() {
        for (uint32_t i = 0; i < m_aliveParticles; ++i){
            m_instanceData[i].position = m_particles[i].position;
            m_instanceData[i].size = m_particles[i].size;
            m_instanceData[i].color = m_particles[i].color;
        }
    }

    void ParticleEmitter::BuildInstanceVertexBuffer() {
       //SR_INFO("BUILD INSTANCE BUFFER");

        BuildInstanceData();

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

        //SR_INFO("VBO {}", m_VBO);

        m_VBO = GetPipeline()->AllocateVBO(
                m_VBO,
                m_maxParticles * m_instanceVertexBuffer.GetLayout().GetStride(),
                m_instanceVertexBuffer.GetRawData());
    }

    void ParticleEmitter::SpawnParticle(){
        if (m_aliveParticles >= m_maxParticles){
            return;
        }

        auto& particle = m_particles[m_aliveParticles];

        particle.position = m_shape->GeneratePosition();
        particle.velocity = m_shape->GenerateDirection() * m_main.startSpeed;

        particle.lifetime = m_main.startLifetime;
        particle.maxLifetime = m_main.startLifetime;
        particle.color = m_main.m_startColor;
        particle.size = m_main.startSize;

        ++m_aliveParticles;
    }

    void ParticleEmitter::UpdateParticle(float_t dt){
        for (uint32_t i = 0; i < m_aliveParticles;){
            auto& particle = m_particles[i];

            particle.velocity.y += m_main.gravity * dt;
            particle.position += particle.velocity * dt;

            particle.lifetime -= dt;

            float_t t = particle.lifetime / particle.maxLifetime;

            particle.color.x = SR_MATH_NS::Lerp(m_main.m_startColor.x, m_main.m_endColor.x, t);
            particle.color.y = SR_MATH_NS::Lerp(m_main.m_startColor.y, m_main.m_endColor.y, t);
            particle.color.z= SR_MATH_NS::Lerp(m_main.m_startColor.z, m_main.m_endColor.z, t);
            particle.color.w = SR_MATH_NS::Lerp(m_main.m_startColor.w, m_main.m_endColor.w, t);

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
        m_shape = new SphereShape();

        //SetRawMesh(SR_UTILS_NS::Path("Samples/diamond.fbx"));

        //auto& buf = GetVertexBuffer(GetVertexLayoutDescription());

        //SR_INFO("VERTICES: {}", buf.GetVertexCount());
        //SR_INFO("INDICES: {}", GetIndices().size());

        InitializeParticle();

        //LoadMesh();

        Super::OnEnable();

        if (auto* pRenderScene = TryGetRenderScene()){
            pRenderScene->GetParticleUpdater().RegisterEmitter(this);
        } else {
            SR_ERROR("NO RENDER SCENE");
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

        //SR_INFO("GET VBO");

        return m_VBO;
    }

    bool ParticleEmitter::Bind() {
        if (m_VBO == SR_ID_INVALID){
            return false;
        }

        //if(m_geometryVBO != SR_ID_INVALID) {
        //    GetPipeline()->BindVBO(m_geometryVBO);
        //}

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
        //SR_INFO("CALCULATE");

        if (!m_isVBODirty) {
            return;
        }

        BuildInstanceVertexBuffer();

        m_isVBODirty = false;
    }

    void ParticleEmitter::UseMaterial(SR_GTYPES_NS::Shader& shader) {
        Super::UseMaterial(shader);
        UseModelMatrix(shader);
        SR_UTILS_NS::StringAtom SHADER_SPRITE_FILL_METHOD = "ISQuad";
        shader.SetInt(SHADER_SPRITE_FILL_METHOD, static_cast<int>(1));
    }

    void ParticleEmitter::UseModelMatrix(SR_GTYPES_NS::Shader& shader) {
        Super::UseModelMatrix(shader);

        if (auto&& pTransform = GetTransform()) SR_LIKELY_ATTRIBUTE {
            shader.SetMat4(SHADER_MODEL_MATRIX, pTransform->GetMatrix());
        }
    }

    const SR_HTYPES_NS::FastMemoryArray<uint32_t>& ParticleEmitter::GetIndices() const {
        SR_TRACY_ZONE;
        return GetRawMesh()->GetIndices(GetMeshId());
    }

    /*void ParticleEmitter::LoadMesh() {
        if (!IsValidMeshId()) {
            return;
        }

        const auto& meshBuffer = GetVertexBuffer(GetVertexLayoutDescription());

        m_geometryVBO = GetPipeline()->AllocateVBO(
                m_geometryVBO,
                meshBuffer.GetVertexCount() *
                meshBuffer.GetLayout().GetStride(),
                meshBuffer.GetRawData()
        );
    }
     */
}
