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
    // class CascadedShadowMapPass;
    // class ShadowMapPass;

    class MeshDrawerPass : public BasePass, public LayerFilterPredicate, public PriorityFilterPredicate { // , public ShaderReplacePredicate
        SR_CLASS()
        using Super = BasePass;
    public:
        using RenderQueuePtr = SR_HTYPES_NS::SharedPtr<RenderQueue>;

    public:
        MeshDrawerPass();
        ~MeshDrawerPass() override;

        bool Init() override;
        void DeInit() override;
        bool Render() override;
        void Update() override;

        SR_NODISCARD bool HasPreRender() const noexcept override { return false; }
        SR_NODISCARD bool HasPostRender() const noexcept override { return false; }
        SR_NODISCARD virtual bool IsNeedUpdate() const noexcept { return false; }
        SR_NODISCARD virtual uint8_t GetMeshDrawerFBOLayers() const noexcept { return 1; }

        virtual void UseUniforms(SR_GTYPES_NS::Shader* pShader, MeshPtr pMesh);
        virtual void UseSharedUniforms(SR_GTYPES_NS::Shader* pShader);
        virtual void UseConstants(SR_GTYPES_NS::Shader* pShader);

        virtual void OnUniformsUpdated() { }

        //SR_NODISCARD ShaderUseInfo ReplaceShader(SR_GTYPES_NS::Shader* pShader) const override;
        SR_NODISCARD bool IsLayerAllowed(SR_UTILS_NS::StringAtom layer) const override;
        SR_NODISCARD bool IsPriorityAllowed(int64_t priority) const override { return true; }

        SR_NODISCARD const std::vector<RenderQueuePtr>& GetRenderQueues() const noexcept { return m_renderQueues; }
        SR_NODISCARD const SR_SRSL_NS::ShaderMacrosParams& GetShaderMacros() const noexcept { return m_shaderMacros; }

    protected:
        SR_NODISCARD RenderStrategy* GetRenderStrategy() const;
        SR_NODISCARD virtual RenderQueuePtr AllocateRenderQueue();

    //private:
        //void ClearOverrideShaders();

    private:
        std::vector<RenderQueuePtr> m_renderQueues;
        SR_HTYPES_NS::Time& m_time;
        SR_SRSL_NS::ShaderMacrosParams m_shaderMacros;

        //ShadowMapPass* m_shadowMapPass = nullptr;
        //CascadedShadowMapPass* m_cascadedShadowMapPass = nullptr;

        /// @property
        std::set<SR_UTILS_NS::StringAtom> m_allowedLayers;
        /// @property
        std::set<SR_UTILS_NS::StringAtom> m_disallowedLayers;
        /// @property
        std::set<std::string> m_shaderDefines;

    };
}

#endif //SR_ENGINE_MESH_DRAWER_PASS_H
