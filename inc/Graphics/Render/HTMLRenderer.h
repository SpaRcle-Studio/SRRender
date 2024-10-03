//
// Created by Monika on 14.08.2024.
//

#ifndef SR_ENGINE_GRAPHICS_HTML_RENDERER_H
#define SR_ENGINE_GRAPHICS_HTML_RENDERER_H

#include <Graphics/Render/HTML/HTMLDrawableElement.h>

namespace SR_GRAPH_NS {
    class HTMLRendererBase : public SR_UTILS_NS::Web::HTMLContainerInterface {
        using Super = SR_UTILS_NS::Web::HTMLContainerInterface;
        struct ShaderInfo {
            SR_GTYPES_NS::Shader::Ptr pShader;

            uint32_t index = 0;

            struct MemInfo {
                Memory::UBOManager::VirtualUBO virtualUBO;
                DescriptorManager::VirtualDescriptorSet virtualDescriptor;
            };

            std::vector<MemInfo> UBOs;
        };
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<HTMLRendererBase>;

    public:
        HTMLRendererBase();

        virtual bool Init();
        virtual void DeInit();

        virtual void Draw();
        virtual void Update();

        void SetCamera(const SR_GTYPES_NS::Camera::Ptr& pCamera) { m_pCamera = pCamera; }
        void SetPipeline(Pipeline* pipeline) { m_pipeline = pipeline; }

    private:
        void get_media_features(litehtml::media_features& media) const override;
        void get_client_rect(litehtml::position& client) const override;

        litehtml::uint_ptr create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) override;
        void delete_font(litehtml::uint_ptr hFont) override;

        void draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer& layer, const litehtml::web_color& color) override;

        bool BeginElement(ShaderInfo& shaderInfo);
        void DrawElement(ShaderInfo& shaderInfo);
        void UpdateElement(ShaderInfo& shaderInfo);
        void EndElement(ShaderInfo& shaderInfo);

    private:
        SR_GRAPH_NS::Memory::UBOManager& m_uboManager;
        SR_GRAPH_NS::DescriptorManager& m_descriptorManager;

        SR_MATH_NS::FVector2 m_viewSize;

        bool m_updateMode = false;
        Pipeline* m_pipeline = nullptr;
        SR_GTYPES_NS::Camera::Ptr m_pCamera = nullptr;

        std::map<SR_UTILS_NS::StringAtom, ShaderInfo> m_shaders;

    };
}

#endif //SR_ENGINE_GRAPHICS_HTML_RENDERER_H
