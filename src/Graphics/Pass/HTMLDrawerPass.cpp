//
// Created by Monika on 15.08.2024.
//

#include <Graphics/Pass/HTMLDrawerPass.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Types/Camera.h>

#include <Utils/Web/HTML/HTMLParser.h>

namespace SR_GRAPH_NS {
    SR_REGISTER_RENDER_PASS(HTMLDrawerPass)

    HTMLDrawerPass::~HTMLDrawerPass() {
        SRAssert2(m_fileWatchers.empty(), "Watchers are not empty!");
    }

    bool HTMLDrawerPass::Load(const SR_XML_NS::Node& passNode) {
        SR_TRACY_ZONE;
        LoadPage(passNode.GetAttribute("Path").ToString());
        return Super::Load(passNode);
    }

    void HTMLDrawerPass::Prepare() {
        if (m_needReloadPage) {
            SR_INFO("HTMLDrawerPass::Prepare() : reloading page \"{}\"...,", m_pagePath.c_str());
            --m_needReloadPage;
            ReloadPage();
            InitRenderer();
            GetPassPipeline()->SetDirty(true);
        }
        Super::Prepare();
    }

    bool HTMLDrawerPass::Render() {
        if (m_pRenderer) {
            m_pRenderer->SetCamera(m_camera);
            m_pRenderer->Draw();
        }
        return true;
    }

    void HTMLDrawerPass::Update() {
        if (m_pRenderer) {
            m_pRenderer->SetCamera(m_camera);
            m_pRenderer->Update();
        }
        BasePass::Update();
    }

    bool HTMLDrawerPass::Init() {
        const bool result = BasePass::Init();
        InitRenderer();
        return result;
    }

    void HTMLDrawerPass::DeInit() {
        if (m_pRenderer) {
            m_pRenderer->DeInit();
        }

        for (auto&& pWatcher : m_fileWatchers) {
            pWatcher->Stop();
        }
        m_fileWatchers.clear();

        Super::DeInit();
    }

    void HTMLDrawerPass::OnResize(const SR_MATH_NS::UVector2& size) {
        if (m_pRenderer) {
            m_pRenderer->SetScreenSize(size);
        }
        Super::OnResize(size);
    }

    bool HTMLDrawerPass::LoadPage(const SR_UTILS_NS::Path& pagePath) {
        if (pagePath.IsEmpty()) {
            SR_ERROR("HTMLDrawerPass::LoadPage() : path is empty!");
            return false;
        }

        m_pagePath = pagePath;

        m_pPage.AutoFree();
        m_pPage = SR_UTILS_NS::Web::HTMLParser::Instance().Parse(m_pagePath);

        if (!m_pPage) {
            SR_ERROR("HTMLDrawerPass::LoadPage() : failed to parse page: {}", m_pagePath.c_str());
        }

        if (m_pPage) {
            for (auto&& pWatcher : m_fileWatchers) {
                pWatcher->Stop();
            }
            m_fileWatchers.clear();
            for (const SR_UTILS_NS::Path& path : m_pPage->GetPaths()) {
                AddWatcher(path);
            }
        }
        else if (m_fileWatchers.empty()) {
            AddWatcher(m_pagePath);
        }

        return m_pPage;
    }

    bool HTMLDrawerPass::ReloadPage() {
        SR_INFO("HTMLDrawerPass::ReloadPage() : {}", m_pagePath.c_str());
        if (!m_pagePath.IsEmpty()) {
            return LoadPage(m_pagePath);
        }
        return false;
    }

    void HTMLDrawerPass::AddWatcher(const SR_UTILS_NS::Path& path) {
        const SR_UTILS_NS::Path fullPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(path);
        for (auto&& pWatcher : m_fileWatchers) {
            if (pWatcher->GetPath() == fullPath) {
                return;
            }
        }
        if (auto&& pWatcher = SR_UTILS_NS::ResourceManager::Instance().StartWatch(fullPath)) {
            pWatcher->SetCallBack([this](SR_UTILS_NS::FileWatcher*) {
                ++m_needReloadPage;
            });
            m_fileWatchers.emplace_back(pWatcher);
        }
    }

    void HTMLDrawerPass::InitRenderer() {
        if (m_pRenderer) {
            m_pRenderer->DeInit();
            m_pRenderer.AutoFree();
        }

        if (m_pPage) {
            const SR_MATH_NS::UVector2 size = m_camera ? m_camera->GetSize() : GetContext()->GetWindowSize();

            m_pRenderer = HTMLRenderer::MakeShared(GetPassPipeline().Get(), m_pPage);
            m_pRenderer->SetScreenSize(size);
            m_pRenderer->Init();
        }
    }
}
