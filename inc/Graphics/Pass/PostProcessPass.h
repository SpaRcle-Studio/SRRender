//
// Created by Monika on 07.08.2022.
//

#ifndef SR_ENGINE_GRAPHICS_POST_PROCESS_PASS_H
#define SR_ENGINE_GRAPHICS_POST_PROCESS_PASS_H

#include <Graphics/Pass/BasePass.h>
#include <Graphics/Pipeline/IShaderProgram.h>

namespace SR_GTYPES_NS {
    class Shader;
}

namespace SR_GRAPH_NS {
    class PostProcessPass : public BasePass {
        SR_CLASS()

        struct Property {
            SR_UTILS_NS::StringAtom id;
            ShaderPropertyVariant data = {};
            ShaderVarType type = ShaderVarType::Unknown;
        };

        using Super = BasePass;
        using Properties = std::vector<Property>;
        using ShaderPtr = SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Shader>;
    public:
        ~PostProcessPass() override;

    public:
        void OnResize(const SR_MATH_NS::UVector2& size) override;
        void OnMultisampleChanged() override;

        bool PreRender() override;
        bool Render() override;
        void Update() override;
        void Prepare() override;

    protected:
        void UseSamplers(SR_GTYPES_NS::Shader* pShader) override;
        void SetRenderTechnique(SR_GRAPH_NS::IRenderTechnique* pRenderTechnique) override;
        void SetShader(const SR_UTILS_NS::Path& shaderPath);
        void DeInit() override;

    protected:
        SR_UTILS_NS::Subscription m_onShaderReloaded;
        int32_t m_virtualDescriptor = SR_ID_INVALID;
        int32_t m_virtualUBO = SR_ID_INVALID;
        bool m_dirtyShader = true;
        ShaderPtr m_shader = nullptr;
        Properties m_properties;

        /// @property
        uint32_t m_vertices = 3;
        /// @property @setter(SetShader)
        /// @customArgs(pick: enabled, filter name: Shader, relative: resources)
        /// @customArg(filter value: srsl)
        SR_UTILS_NS::Path m_shaderPath;
        /// @property
        SamplersPassData m_samplers;

    };
}

#endif //SR_ENGINE_GRAPHICS_POST_PROCESS_PASS_H
