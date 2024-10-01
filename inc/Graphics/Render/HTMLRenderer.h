//
// Created by Monika on 14.08.2024.
//

#ifndef SR_ENGINE_GRAPHICS_HTML_RENDERER_H
#define SR_ENGINE_GRAPHICS_HTML_RENDERER_H

#include <Utils/Web/HTML/HTML.h>

#include <Graphics/Types/Shader.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Types/Texture.h>

namespace SR_GRAPH_NS {
    struct HTMLRendererUpdateContext {
        SR_MATH_NS::FVector2 resolution;
        SR_MATH_NS::FVector2 size;
        SR_MATH_NS::FVector2 offset;
    };

    class HTMLDrawableElement : public SR_UTILS_NS::NonCopyable {
    public:
        ~HTMLDrawableElement() override;

        void SetShader(SR_GTYPES_NS::Shader::Ptr pShader);

        void SetPage(SR_UTILS_NS::Web::HTMLPage* pPage) { m_pPage = pPage; }
        void SetNodeId(uint64_t id) { m_nodeId = id; }
        void SetPipeline(Pipeline* pPipeline) { m_pipeline = pPipeline; }
        void SetTexture(SR_GTYPES_NS::Texture::Ptr pTexture);

        SR_NODISCARD SR_GTYPES_NS::Shader::Ptr GetShader() const { return m_pShader; }

        void Draw();
        void Update(const HTMLRendererUpdateContext& parentContext);

    private:
        SR_GTYPES_NS::Texture::Ptr m_pTexture = nullptr;
        SR_GTYPES_NS::Shader::Ptr m_pShader = nullptr;

        uint64_t m_nodeId = SR_ID_INVALID;
        SR_UTILS_NS::Web::HTMLPage* m_pPage = nullptr;

        bool m_dirtyMaterial = true;
        int32_t m_virtualUBO = SR_ID_INVALID;
        int32_t m_virtualDescriptor = SR_ID_INVALID;
        Pipeline* m_pipeline = nullptr;

    };

    class HTMLRenderer : public SR_HTYPES_NS::SharedPtr<HTMLRenderer> {
        using Super = SR_HTYPES_NS::SharedPtr<HTMLRenderer>;
    public:
        HTMLRenderer(Pipeline* pPipeline, SR_UTILS_NS::Web::HTMLPage::Ptr pPage);
        ~HTMLRenderer();

        bool Init();
        void DeInit();

        void Draw();
        void Update();

        void SetScreenSize(const SR_MATH_NS::UVector2& size);
        void SetCamera(const SR_GTYPES_NS::Camera::Ptr& pCamera) { m_pCamera = pCamera; }

    private:
        void PrepareNode(SR_UTILS_NS::Web::HTMLNode* pNode);
        void DrawNode(const SR_UTILS_NS::Web::HTMLNode* pNode);
        void UpdateNode(const SR_UTILS_NS::Web::HTMLNode* pNode, const HTMLRendererUpdateContext& parentContext);

    private:
        SR_UTILS_NS::Web::HTMLPage::Ptr m_pPage;
        Pipeline* m_pipeline = nullptr;
        SR_GTYPES_NS::Camera::Ptr m_pCamera = nullptr;

        std::vector<HTMLDrawableElement*> m_drawableElements;
        std::unordered_map<SR_UTILS_NS::StringAtom, SR_GTYPES_NS::Shader::Ptr> m_shaders;

    };
}

#endif //SR_ENGINE_GRAPHICS_HTML_RENDERER_H
