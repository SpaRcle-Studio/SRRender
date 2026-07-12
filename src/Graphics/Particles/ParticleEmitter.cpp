//
// Created by Monika on 11.04.2026.
//

#include <Graphics/Particles/ParticleEmitter.h>
#include <Graphics/Particles/ParticleData.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Memory/DescriptorManager.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Material/BaseMaterial.h>

#include <Utils/Math/Vector3.h>
#include <Utils/Math/Vector4.h>
#include <Utils/Common/Numeric.h>
#include <Utils/Types/RawMesh.h>
#include <Utils/FileSystem/PathDataAccessor.h>

#include <Codegen/ParticleEmitter.generated.hpp>

namespace SR_GRAPH_NS {
    static const auto ParticleEmitterVertexLayout = SR_UTILS_NS::VertexLayoutDescription()
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Position1, SR_UTILS_NS::VertexAttributeFormat::Float32, 3)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Color0, SR_UTILS_NS::VertexAttributeFormat::Float32, 4)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Custom0, SR_UTILS_NS::VertexAttributeFormat::Float32, 1)
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Custom1, SR_UTILS_NS::VertexAttributeFormat::Float32, 3)
        .SetInstanced(true);

    /// Geometry data
    static const auto ParticleEmitterGeometryVertexLayout = SR_UTILS_NS::VertexLayoutDescription()
        .AddAttribute(SR_UTILS_NS::VertexAttribute::Position, SR_UTILS_NS::VertexAttributeFormat::Float32, 3)
    ;

    void ParticleEmitter::InitializeParticle() {
        m_particles.resize(m_maxParticles);
        m_instanceData.resize(m_particles.size());

        m_instanceVertexBuffer.SetLayout(ParticleEmitterVertexLayout);
        m_instanceVertexBuffer.Allocate(m_maxParticles);

        m_VBO = GetPipeline()->AllocateVBO(
                m_VBO,
                m_maxParticles * m_instanceVertexBuffer.GetLayout().GetStride(),
                m_instanceVertexBuffer.GetRawData());
    }

    void ParticleEmitter::Draw() {
        if (m_aliveParticles == 0) {
            return;
        }

        Calculate();

        GetPipeline()->SetDrawInstancesCount(m_aliveParticles);

        if (IsValidMeshId()) {
            GetPipeline()->BindVBO(m_geometryVBO, 1, VertexInputRate::Vertex);
            GetPipeline()->BindIBO(m_geometryIBO);
        }

        DrawRenderObject(
            this,
            IsValidMeshId() ? GetIndices().size() : 6,
            m_virtualUBO,
            m_virtualDescriptor,
            m_dirtyMaterial,
            m_hasErrors
        );

        GetPipeline()->BindVBO(SR_ID_INVALID, 1, VertexInputRate::Vertex);

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
            m_instanceData[i].rotation = m_particles[i].rotation;
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
            m_instanceVertexBuffer.SetVertexT(i, SR_UTILS_NS::VertexAttribute::Custom1,
                                              m_instanceData[i].rotation);
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
        particle.direction = m_shape->GenerateDirection();
        particle.velocity = particle.direction * SR_UTILS_NS::Random::Instance().Float(m_main.startMinSpeed, m_main.startMaxSpeed);

        particle.lifetime = SR_UTILS_NS::Random::Instance().Float(m_main.startMinLifetime, m_main.startMaxLifetime);
        particle.maxLifetime = particle.lifetime;

        particle.color = m_main.m_startColor;

        particle.startSize = SR_UTILS_NS::Random::Instance().Float(m_main.startMinSize, m_main.startMaxSize);
        particle.size = particle.startSize;

        particle.rotation = SR_MATH_NS::FVector3(0.0f);
        particle.rotationSpeed = m_main.startRotationSpeed;


        ++m_aliveParticles;
    }

    void ParticleEmitter::UpdateParticle(float_t dt){
        for (uint32_t i = 0; i < m_aliveParticles;){
            auto& particle = m_particles[i];

            float_t t = particle.lifetime / particle.maxLifetime;

            particle.velocity.y += m_main.gravity * dt;
            particle.velocity += m_main.directionVelosity * dt;
            particle.position += particle.velocity * dt;

            particle.lifetime -= dt;

            particle.rotation += particle.rotationSpeed * dt;

            particle.size = SR_MATH_NS::Lerp(particle.startSize, m_main.endSize, 1.0f - t);

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
        m_emitterTimer += dt;
        if(m_emitterTimer >= m_emission.duration){
            if(m_emission.looping) {
                m_emitterTimer -= m_emission.duration;
            } else {
                canSpawn = false;
            }
        }

        const float_t spawnInterval = 1.0f / m_emission.rateOverTime;

        while (canSpawn && m_spawnTimer >= spawnInterval){
            SpawnParticle();
            m_spawnTimer -= spawnInterval;
        }

        UpdateParticle(dt);

        m_isParticlesVBODirty = true;
    }

    void ParticleEmitter::OnEnable(){
        m_shape = new ConusShape();

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

    void ParticleEmitter::OnDetached() {
        Super::OnDetached();
        if (auto* pRenderScene = TryGetRenderScene()) {
            pRenderScene->GetParticleUpdater().RemoveEmitter(this);
        }
    }

    SR_UTILS_NS::VertexLayoutDescriptionsRef ParticleEmitter::GetShaderVertexLayoutDescriptions() const noexcept {
        if (IsValidMeshId()) {
            static const std::array<SR_UTILS_NS::VertexLayoutDescription, 2> layouts = { ParticleEmitterVertexLayout, ParticleEmitterGeometryVertexLayout };
            return SR_UTILS_NS::VertexLayoutDescriptionsRef(layouts);
        }
        return SR_UTILS_NS::VertexLayoutDescriptionsRef(ParticleEmitterVertexLayout);
    }

    std::optional<int32_t> ParticleEmitter::GetVBO() const {
        const_cast<ParticleEmitter&>(*this).Calculate();
        if (m_VBO == SR_ID_INVALID){
            return std::nullopt;
        }
        return m_VBO;
    }

    std::optional<int32_t> ParticleEmitter::GetIBO() const {
        const_cast<ParticleEmitter&>(*this).Calculate();
        if (m_geometryIBO == SR_ID_INVALID){
            return std::nullopt;
        }
        return m_geometryIBO;
    }

    bool ParticleEmitter::Bind() {
        Calculate();
        if (m_VBO == SR_ID_INVALID) {
            return false;
        }
        GetPipeline()->BindVBO(m_VBO, 0, VertexInputRate::Instance);
        return true;
    }

    void ParticleEmitter::FreeVideoMemory() {
        Super::FreeVideoMemory();

        if (m_VBO != SR_ID_INVALID){
            GetPipeline()->FreeVBO(&m_VBO);
        }
        if (m_geometryVBO != SR_ID_INVALID){
            GetPipeline()->FreeVBO(&m_geometryVBO);
        }
        if (m_geometryIBO != SR_ID_INVALID){
            GetPipeline()->FreeIBO(&m_geometryIBO);
        }

        auto&& uboManager = Memory::UBOManager::Instance();
        auto&& descriptorManager = SR_GRAPH_NS::DescriptorManager::Instance();

        if (m_virtualUBO != SR_ID_INVALID && !uboManager.FreeUBO(&m_virtualUBO)) {
            SR_ERROR("ParticleEmitter::FreeVideoMemory() : failed to free virtual uniform buffer object!");
        }

        if (m_virtualDescriptor != SR_ID_INVALID && !descriptorManager.FreeDescriptorSet(&m_virtualDescriptor)) {
            SR_ERROR("ParticleEmitter::FreeVideoMemory() : failed to free virtual descriptor set!");
        }
    }

    void ParticleEmitter::Calculate() {
        if (m_isParticlesVBODirty) {
            BuildInstanceVertexBuffer();
            m_isParticlesVBODirty = false;
        }

        if (m_isGeometryVBODirty && IsValidMeshId()) {
            if (m_geometryIBO != SR_ID_INVALID) {
                GetPipeline()->FreeIBO(&m_geometryIBO);
            }
            const auto& meshBuffer = GetVertexBuffer(ParticleEmitterGeometryVertexLayout);
            auto&& indices = GetIndices();
            m_geometryVBO = GetPipeline()->AllocateVBO(m_geometryVBO, meshBuffer.GetDataSize(), meshBuffer.GetRawData());
            m_geometryIBO = GetPipeline()->AllocateIBO((void *) indices.data(), sizeof(uint32_t), indices.size(), m_geometryVBO);
            m_isGeometryVBODirty = false;
        }
    }

    void ParticleEmitter::UseMaterial(SR_GTYPES_NS::Shader& shader) {
        Super::UseMaterial(shader);
        UseModelMatrix(shader);
        static const SR_UTILS_NS::StringAtom id = "ISQuad";
        shader.SetInt(id, static_cast<int>(!IsValidMeshId()));
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

    void ParticleEmitter::OnRawMeshChanged() {
        IRawMeshHolder::OnRawMeshChanged();
        ReRegisterRenderObject();
        MarkMaterialDirty();
        m_isGeometryVBODirty = true;
    }
}
