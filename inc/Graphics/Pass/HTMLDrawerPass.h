//
// Created by Monika on 15.08.2024.
//

#ifndef SR_ENGINE_HTML_DRAWER_PASS_H
#define SR_ENGINE_HTML_DRAWER_PASS_H

#include <Graphics/Pass/BasePass.h>
#include <Graphics/Render/HTMLRenderer.h>

namespace SR_GRAPH_NS {
#ifdef SR_COMMON_LITEHTML
    class HTMLDrawerPass : public BasePass {
        SR_REGISTER_LOGICAL_NODE(HTMLDrawerPass, HTML Drawer Pass, { "Passes" })
        using Super = BasePass;
    public:
        bool Load(const SR_XML_NS::Node& passNode) override;

        bool Render() override;
        void Update() override;

        bool Init() override;
        void DeInit() override;

        void OnResize(const SR_MATH_NS::UVector2& size) override;

    private:
        HTMLRenderer::Ptr m_pRenderer = nullptr;
        SR_UTILS_NS::Web::HTMLPage::Ptr m_pPage = nullptr;

    };
#endif
}

#endif //SR_ENGINE_HTML_DRAWER_PASS_H