//
// Created by Monika on 15.08.2024.
//

#ifndef SR_ENGINE_HTML_DRAWER_PASS_H
#define SR_ENGINE_HTML_DRAWER_PASS_H

#include <Graphics/Pass/BasePass.h>
#include <Graphics/Render/HTMLRenderer.h>

namespace SR_GRAPH_NS {
    class HTMLDrawerPass : public BasePass {
        SR_REGISTER_LOGICAL_NODE(HTMLDrawerPass, HTML Drawer Pass, { "Passes" })
        using Super = BasePass;
    public:
        ~HTMLDrawerPass() override;

        bool Load(const SR_XML_NS::Node& passNode) override;

        void Prepare() override;
        bool Render() override;
        void Update() override;

        bool Init() override;
        void DeInit() override;

        void OnResize(const SR_MATH_NS::UVector2& size) override;

    private:
        bool LoadPage(const SR_UTILS_NS::Path& path);
        void UnloadPage();
        bool ReloadPage();
        void AddWatcher(const SR_UTILS_NS::Path& path);
        void InitRenderer();

    private:
        SR_UTILS_NS::Path m_pagePath;
        std::vector<SR_UTILS_NS::FileWatcher::Ptr> m_fileWatchers;
        SR_UTILS_NS::Web::HTMLPage::Ptr m_pPage = nullptr;
        std::atomic<uint16_t> m_needReloadPage = false;

    };
}

#endif //SR_ENGINE_HTML_DRAWER_PASS_H