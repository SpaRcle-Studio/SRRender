//
// Created by Monika on 18.01.2024.
//

#ifndef SR_ENGINE_MESH_DRAWER_PASS_H
#define SR_ENGINE_MESH_DRAWER_PASS_H

#include <Graphics/Pass/BasePass.h>
#include <Graphics/Render/RenderPredicates.h>
#include <Graphics/Pipeline/IShaderProgram.h>
#include <Graphics/SRSL/ShaderType.h>

namespace SR_GRAPH_NS {
    class RenderStrategy;
    class RenderQueue;

    struct MeshDrawerSharedUniforms : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        bool camera = true;
        /// @property
        bool time = true;
        /// @property
        bool light = true;
        /// @property
        std::set<SR_UTILS_NS::StringAtom> useFromPass;

    };

    struct MeshDrawerMaterialUniforms : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        bool useMaterial = true;
        /// @property
        /// @propertyCondition(This.useMaterial == false)
        bool modelMatrix = true;

    };

    struct MeshDrawerUniforms : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// @property
        MeshDrawerSharedUniforms shared;
        /// @property
        MeshDrawerMaterialUniforms material;

    };

    class MeshDrawerPass : public BasePass, public LayerFilterPredicate, public PriorityFilterPredicate {
        SR_CLASS()
        using Super = BasePass;
    public:
        using RenderQueuePtr = SR_HTYPES_NS::SharedPtr<RenderQueue>;

    public:
        MeshDrawerPass();
        ~MeshDrawerPass() override;

        bool Init() override;
        void DeInit() override;
        void Prepare() override;
        bool Render() override;
        void Update() override;

        SR_NODISCARD bool HasPreRender() const noexcept override { return false; }
        SR_NODISCARD bool HasPostRender() const noexcept override { return false; }
        SR_NODISCARD virtual bool IsNeedUpdate() const noexcept { return false; }

        void SetRenderTechnique(SR_GRAPH_NS::IRenderTechnique* pRenderTechnique) override;

        virtual void UseUniforms(SR_GTYPES_NS::Shader* pShader, MeshPtr pMesh);
        virtual void UseSharedUniforms(SR_GTYPES_NS::Shader* pShader);
        virtual void UseConstants(SR_GTYPES_NS::Shader* pShader);
        void UseSamplers(SR_GTYPES_NS::Shader* pShader) override;

        void OnMultisampleChanged() override;
        void OnResize(const SR_MATH_NS::UVector2& size) override;
        void SetRenderLayers(uint8_t layers) { m_renderLayers = layers; }

        SR_NODISCARD bool IsLayerAllowed(SR_UTILS_NS::StringAtom layer) const override;
        SR_NODISCARD bool IsPriorityAllowed(int64_t priority) const override { return true; }

        SR_NODISCARD const std::vector<RenderQueuePtr>& GetRenderQueues() const noexcept { return m_renderQueues; }
        SR_NODISCARD const SR_SRSL_NS::ShaderMacrosParams& GetShaderMacros() const noexcept { return m_shaderMacros; }
        SR_NODISCARD uint8_t GetLayersCount() const noexcept { return m_renderLayers; }

    protected:
        SR_NODISCARD RenderStrategy* GetRenderStrategy() const;
        SR_NODISCARD virtual RenderQueuePtr AllocateRenderQueue();
        virtual void UpdateShaderDefines(SR_SRSL_NS::ShaderMacrosParams& defines) const { }

    protected:
        SR_SRSL_NS::ShaderMacrosParams m_shaderMacros;

    private:
        std::vector<RenderQueuePtr> m_renderQueues;
        SR_HTYPES_NS::Time& m_time;
        std::vector<BasePass*> m_useSharedFromPass;

        /// @property
        uint8_t m_renderLayers = 1;
        /// @property
        std::set<SR_UTILS_NS::StringAtom> m_allowedLayers;
        /// @property
        std::set<SR_UTILS_NS::StringAtom> m_disallowedLayers;
        /// @property
        std::set<std::string> m_shaderDefines;
        /// @property
        SamplersPassData m_samplers;
        /// @property
        MeshDrawerUniforms m_uniforms;

    };
}

#endif //SR_ENGINE_MESH_DRAWER_PASS_H
