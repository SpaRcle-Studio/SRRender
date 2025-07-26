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
    class PostProcessPass : public BasePass {
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
        bool Load(const SR_XML_NS::Node& passNode) override;

        void OnResize(const SR_MATH_NS::UVector2& size) override;
        void OnMultisampleChanged() override;

        bool PreRender() override;
        bool Render() override;
        void Update() override;

    protected:
        void SetShader(const SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Shader>& pShader);

        void DeInit() override;

    protected:
        SR_UTILS_NS::Subscription m_onShaderReloaded;
        int32_t m_virtualDescriptor = SR_ID_INVALID;
        int32_t m_virtualUBO = SR_ID_INVALID;
        bool m_dirtyShader = true;
        ShaderPtr m_shader = nullptr;
        Properties m_properties;
        uint32_t m_vertices = 0;

    };
}

#endif //SR_ENGINE_GRAPHICS_POST_PROCESS_PASS_H
