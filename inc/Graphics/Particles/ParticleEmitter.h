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
#include <Graphics/Particles/ParticleEmissionModule.h>

#include <Graphics/Types/Geometry/IndexedMesh.h>
#include <Graphics/Types/Geometry/IndexedMesh.h>

#include <Utils/Types/IRawMeshHolder.h>
#include <Utils/ECS/Component.h>

namespace SR_GRAPH_NS {
    /// @category(Render.Particles)
    class ParticleEmitter : public SR_GTYPES_NS::IRenderComponent, public SR_HTYPES_NS::IRawMeshHolder {
        SR_CLASS()
        using Super = SR_GTYPES_NS::IRenderComponent;
    public:
        void InitializeParticle();
        void BuildInstanceData();
        void BuildInstanceVertexBuffer();
        void SpawnParticle();
        void UpdateParticle(float_t dt);
        void UpdateEmitter(float_t dt);
        void KillParticle(uint32_t index);
        void OnEnable() override;
        void OnDisable() override;
        void OnDetached() override;

        void FreeVideoMemory() override;
        bool Bind() override;

        void Draw() override;
        bool ExecuteInEditMode() const override;

        void UseMaterial(SR_GTYPES_NS::Shader& shader) override;
        void UseModelMatrix(SR_GTYPES_NS::Shader& shader) override;

        void Calculate();

        void OnRawMeshChanged() override;

        SR_NODISCARD std::optional<int32_t> GetVBO() const override;
        SR_NODISCARD std::optional<int32_t> GetIBO() const override;
        SR_NODISCARD int32_t GetVirtualUBO() const override { return m_virtualUBO; }

        const SR_HTYPES_NS::FastMemoryArray<uint32_t>& GetIndices() const;
        SR_UTILS_NS::VertexLayoutDescriptionsRef GetShaderVertexLayoutDescriptions() const noexcept override;

    private:
        /// @virtualProperty(geometryName) @getter(GetGeometryName) @dontSave @readOnly
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(meshPath) @getter(GetMeshPath) @setter(SetRawMesh)
        /// @customArgs(pick: enabled, filter name: Meshes, relative: resources)
        /// @customArg(filter value: fbx,blend,obj,pmx,stl,dae)
        SR_VIRTUAL_PROPERTY
        /// @virtualProperty(meshId) @getter(GetMeshId) @setter(SetMeshId)
        SR_VIRTUAL_PROPERTY

        /// Выделяет инстанс-буферы для всех кадров свапчейна.
        void AllocateInstanceVBOs();

        /// Инстанс-буфер, соответствующий текущему кадру свапчейна.
        SR_NODISCARD int32_t GetCurrentVBO() const;
        SR_NODISCARD uint32_t GetCurrentFrameSlot() const;
        SR_NODISCARD uint32_t GetInstanceVBOsCount() const;

        SR_NODISCARD static std::array<int32_t, SR_MAX_FRAMES_IN_FLIGHT> MakeInvalidVBOs() noexcept {
            std::array<int32_t, SR_MAX_FRAMES_IN_FLIGHT> vbos{};
            vbos.fill(SR_ID_INVALID);
            return vbos;
        }

        SR_HTYPES_NS::FastMemoryArray<ParticleData> m_particles;
        SR_HTYPES_NS::FastMemoryArray<ParticleInstanceData> m_instanceData;
        SR_UTILS_NS::VertexDataBuffer m_instanceVertexBuffer;

        uint32_t m_maxParticles = 1000;
        uint32_t m_aliveParticles = 0;
        ///@property
        ParticleEmissionModule m_emission;
        bool canSpawn = true;
        float_t m_spawnTimer = 0.0f;
        float_t m_emitterTimer = 0.0f;

        /// Отдельный инстанс-буфер на каждый кадр свапчейна.
        /// Командные буферы кешируются, поэтому в буфер кадра, который ещё читает GPU, писать нельзя.
        std::array<int32_t, SR_MAX_FRAMES_IN_FLIGHT> m_VBOs = MakeInvalidVBOs();

        int32_t m_virtualUBO = SR_ID_INVALID;
        int32_t m_virtualDescriptor = SR_ID_INVALID;

        int32_t m_geometryVBO = SR_ID_INVALID;
        int32_t m_geometryIBO = SR_ID_INVALID;

        SR_UTILS_NS::VertexDataBuffer m_geometryBuffer;

        bool m_isGeometryVBODirty = true;
        /// Данные частиц меняются каждый кадр, поэтому грязными помечаются буферы всех кадров,
        /// а каждый кадр обновляет только свой.
        std::bitset<SR_MAX_FRAMES_IN_FLIGHT> m_particlesVBODirtyFrames;
        ParticleMainModule m_main;
        /// @property @notNull
        ParticleShape::Ptr m_shape;
        //ParticleRenderMode m_renderer = ParticleRenderMode::Billboard;
    };
}

#endif //SR_ENGINE_GRAPHICS_PARTICLE_EMITTER_H
