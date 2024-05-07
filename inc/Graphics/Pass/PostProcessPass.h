//
// Created by Monika on 07.08.2022.
//

#ifndef SR_ENGINE_GRAPHICS_POST_PROCESS_PASS_H
#define SR_ENGINE_GRAPHICS_POST_PROCESS_PASS_H

#include <Graphics/Pass/BasePass.h>
#include <Graphics/Pipeline/IShaderProgram.h>

namespace SR_GTYPES_NS {
    class Shader;
    class Framebuffer;
}

namespace SR_GRAPH_NS {
    class PostProcessPass : public BasePass, public ISamplersPass {
        SR_REGISTER_LOGICAL_NODE(PostProcessPass, Post Process Pass, { "Passes" })
        struct Property {
            SR_UTILS_NS::StringAtom id;
            ShaderPropertyVariant data = {};
            ShaderVarType type = ShaderVarType::Unknown;
        };

        using Super = BasePass;
        using Properties = std::vector<Property>;
        using ShaderPtr = SR_GTYPES_NS::Shader*;
    public:
        ~PostProcessPass() override;

    public:
        bool Load(const SR_XML_NS::Node& passNode) override;

        void OnResize(const SR_MATH_NS::UVector2& size) override;
        void OnMultisampleChanged() override;

        void Prepare() override;
        bool PreRender() override;
        bool Render() override;
        void Update() override;

        void OnResourceUpdated(SR_UTILS_NS::ResourceContainer* pContainer, int32_t depth) override;

    protected:
        void SetShader(SR_GTYPES_NS::Shader* pShader);
        void SetRenderTechnique(IRenderTechnique* pRenderTechnique) override;

        void DeInit() override;

    protected:
        int32_t m_virtualDescriptor = SR_ID_INVALID;
        int32_t m_virtualUBO = SR_ID_INVALID;
        bool m_dirtyShader = true;
        ShaderPtr m_shader = nullptr;
        Properties m_properties;
        uint32_t m_vertices = 0;

    };
}

#endif //SR_ENGINE_GRAPHICS_POST_PROCESS_PASS_H
