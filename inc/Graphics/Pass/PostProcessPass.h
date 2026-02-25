//
// Created by Monika on 07.08.2022.
//

#ifndef SR_ENGINE_GRAPHICS_POST_PROCESS_PASS_H
#define SR_ENGINE_GRAPHICS_POST_PROCESS_PASS_H

#include <Graphics/Pass/BasePass.h>
#include <Graphics/Pipeline/IShaderProgram.h>
#include <Graphics/Material/MaterialData.h>
#include <Graphics/Material/UniqueMaterial.h>

namespace SR_GTYPES_NS {
    class Shader;
}

namespace SR_GRAPH_NS {
    class PostProcessPass : public BasePass {
        SR_CLASS()
        using Super = BasePass;
        using ShaderPtr = SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Shader>;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<PostProcessPass>;

    public:
        ~PostProcessPass() override;

    public:
        bool Init() override;
        void OnResize(const SR_MATH_NS::UVector2& size) override;
        void OnMultisampleChanged() override;
        void SetShader(const SR_UTILS_NS::Path& path);

        bool PreRender() override;
        bool Render() override;
        void Update() override;
        bool Prepare() override;

        SR_NODISCARD const BaseMaterial::Ptr& GetMaterial() const { return m_material; }

    protected:
        void UseSamplers(SR_GTYPES_NS::Shader& shader) override;
        void SetRenderTechnique(SR_GRAPH_NS::IRenderTechnique* pRenderTechnique) override;
        void DeInit() override;

    protected:
        SR_UTILS_NS::Subscription m_onShaderReloaded;
        int32_t m_virtualDescriptor = SR_ID_INVALID;
        int32_t m_virtualUBO = SR_ID_INVALID;
        bool m_dirtyShader = true;
        SR_SRSL_NS::ShaderMacrosParams m_shaderMacros;

        /// @property
        uint32_t m_vertices = 3;
        /// @property
        SamplersPassData m_samplers;
        /// @property
        BaseMaterial::Ptr m_material;

    };
}

#endif //SR_ENGINE_GRAPHICS_POST_PROCESS_PASS_H
