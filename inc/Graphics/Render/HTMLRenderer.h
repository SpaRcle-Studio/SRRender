//
// Created by Monika on 14.08.2024.
//

#ifndef SR_ENGINE_GRAPHICS_HTML_RENDERER_H
#define SR_ENGINE_GRAPHICS_HTML_RENDERER_H

#include <Graphics/Render/HTML/HTMLDrawableElement.h>

namespace SR_GRAPH_NS {
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

        HTMLRendererUpdateResult UpdateNode(const SR_UTILS_NS::Web::HTMLNode* pNode, const HTMLRendererUpdateContext& parentContext);

    private:
        SR_UTILS_NS::Web::HTMLPage::Ptr m_pPage;
        Pipeline* m_pipeline = nullptr;
        SR_GTYPES_NS::Camera::Ptr m_pCamera = nullptr;

        std::vector<HTMLDrawableElement*> m_drawableElements;
        std::unordered_map<SR_UTILS_NS::StringAtom, SR_GTYPES_NS::Shader::Ptr> m_shaders;

    };
}

#endif //SR_ENGINE_GRAPHICS_HTML_RENDERER_H
