//
// Created by Monika on 18.01.2024.
//

#ifndef SR_ENGINE_MESH_DRAWER_PASS_H
#define SR_ENGINE_MESH_DRAWER_PASS_H

#include <Graphics/Pass/BasePass.h>
#include <Graphics/Render/RenderPredicates.h>
#include <Graphics/Pipeline/ShaderUtils.h>
#include <Graphics/SRSL/ShaderType.h>
#include <Graphics/Loaders/SRSL.h>

namespace SR_GRAPH_NS {
    struct Frustum;

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
        using Ptr = SR_HTYPES_NS::SharedPtr<MeshDrawerPass>;
        using RenderQueuePtr = SR_HTYPES_NS::SharedPtr<RenderQueue>;

    public:
        MeshDrawerPass();

        bool PreInit() override;
        bool Init() override;
        void DeInit() override;
        bool Prepare() override;
        bool Render() override;
        void Update() override;

        bool UpdateFrustum() override;

        SR_NODISCARD bool HasPreRender() const noexcept override { return false; }
        SR_NODISCARD bool HasPostRender() const noexcept override { return false; }
        SR_NODISCARD bool IsFrustumCullingEnabled() const noexcept { return m_frustumCulling; }
        SR_NODISCARD virtual bool IsNeedUpdate() const noexcept { return false; }

        void SetRenderTechnique(SR_GRAPH_NS::IRenderTechnique* pRenderTechnique) override;

        virtual void UseUniforms(SR_GTYPES_NS::Shader& shader, MeshPtr pMesh);
        virtual void UseSharedUniforms(SR_GTYPES_NS::Shader& shader);
        virtual void UseConstants(SR_GTYPES_NS::Shader& shader);
        void UseSamplers(SR_GTYPES_NS::Shader& shader) override;

        void OnMultisampleChanged() override;
        void OnResize(const SR_MATH_NS::UVector2& size) override;
        void SetRenderLayers(uint8_t layers);
        void SetFrustumCulling(bool enabled) { m_frustumCulling = enabled; }
        void AddShaderDefine(const std::string& define) { m_shaderDefines.insert(define); }

        SR_NODISCARD bool IsLayerAllowed(SR_UTILS_NS::StringAtom layer) const override;
        SR_NODISCARD bool IsPriorityAllowed(int64_t priority) const override { return true; }

        SR_NODISCARD const std::vector<RenderQueuePtr>& GetRenderQueues() const noexcept { return m_renderQueues; }
        SR_NODISCARD const SR_SRSL_NS::ShaderParams& GetShaderParams() const noexcept { return m_shaderParams; }
        SR_NODISCARD uint8_t GetLayersCount() const noexcept { return m_renderLayers; }
        SR_NODISCARD const RenderQueuePtr& GetRenderQueue(uint32_t index) const;
        SR_NODISCARD std::set<SR_UTILS_NS::StringAtom>& GetAllowedLayers() { return m_allowedLayers; }
        SR_NODISCARD std::set<SR_UTILS_NS::StringAtom>& GetDisallowedLayers() { return m_disallowedLayers; }
        SR_NODISCARD SamplersPassData& GetSamplersData() { return m_samplers; }
        SR_NODISCARD MeshDrawerUniforms& GetUniformsData() { return m_uniforms; }

    protected:
        SR_NODISCARD virtual const Frustum& GetFrustum(uint32_t renderLayer, bool& isAvailable) const;
        SR_NODISCARD RenderStrategy* GetRenderStrategy() const;
        SR_NODISCARD virtual RenderQueuePtr AllocateRenderQueue(uint32_t index);
        virtual void UpdateShaderDefines(SR_SRSL_NS::ShaderParams& defines) const { }

    protected:
        SR_SRSL_NS::ShaderParams m_shaderParams;

    private:
        std::vector<RenderQueuePtr> m_renderQueues;
        SR_HTYPES_NS::Time& m_time;
        std::vector<BasePass*> m_useSharedFromPass;
        bool m_valid = true;

        /// @property
        bool m_frustumCulling = true;
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
