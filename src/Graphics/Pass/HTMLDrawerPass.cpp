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
        SRAssert2(!m_pPage, "Page is not empty!");
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
        if (!m_pPage) {
            return false;
        }

        if (const auto pRenderer = m_pPage->GetContainer().PolymorphicCast<HTMLRenderContainer>()) {
            pRenderer->SetCamera(m_camera);

            const SR_MATH_NS::UVector2 size = m_camera ? m_camera->GetSize() : GetContext()->GetWindowSize();
            m_pPage->GetDocument()->render(static_cast<int32_t>(size.x));

            pRenderer->Draw();
            return true;
        }
        return false;
    }

    void HTMLDrawerPass::Update() {
        if (!m_pPage) {
            return;
        }

        if (const auto pRenderer = m_pPage->GetContainer().PolymorphicCast<HTMLRenderContainer>()) {
            pRenderer->SetCamera(m_camera);
            pRenderer->Update();
        }
        Super::Update();
    }

    bool HTMLDrawerPass::Init() {
        const bool result = Super::Init();
        InitRenderer();
        return result;
    }

    void HTMLDrawerPass::DeInit() {
        UnloadPage();

        for (auto&& pWatcher : m_fileWatchers) {
            pWatcher->Stop();
        }
        m_fileWatchers.clear();

        Super::DeInit();
    }

    void HTMLDrawerPass::OnResize(const SR_MATH_NS::UVector2& size) {
        if (!m_pPage) {
            return;
        }

        m_pPage->GetDocument()->media_changed();
        m_pPage->GetDocument()->render(static_cast<int32_t>(size.x));

        Super::OnResize(size);
    }

    bool HTMLDrawerPass::LoadPage(const SR_UTILS_NS::Path& pagePath) {
        if (pagePath.IsEmpty()) {
            SR_ERROR("HTMLDrawerPass::LoadPage() : path is empty!");
            return false;
        }

        UnloadPage();

        m_pagePath = pagePath;

        HTMLRenderContainer::Ptr pContainer = new HTMLRenderContainer();
        pContainer->SetCamera(m_camera);
        pContainer->SetPipeline(GetPassPipeline().Get());

        m_pPage = SR_UTILS_NS::Web::HTMLPage::Load(m_pagePath,
            pContainer.PolymorphicCast<SR_UTILS_NS::Web::HTMLContainerInterface>());

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
            AddWatcher(SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(m_pagePath));
        }

        return m_pPage;
    }

    void HTMLDrawerPass::UnloadPage() {
        if (m_pPage) {
            if (auto&& pContainer = m_pPage->GetContainer().PolymorphicCast<HTMLRenderContainer>()) {
                pContainer->DeInit();
            }
            m_pPage.AutoFree();
            m_pPage = nullptr;
        }
    }

    bool HTMLDrawerPass::ReloadPage() {
        SR_INFO("HTMLDrawerPass::ReloadPage() : {}", m_pagePath.c_str());
        if (!m_pagePath.IsEmpty()) {
            return LoadPage(m_pagePath);
        }
        return false;
    }

    void HTMLDrawerPass::AddWatcher(const SR_UTILS_NS::Path& path) {
        for (auto&& pWatcher : m_fileWatchers) {
            if (pWatcher->GetPath() == path) {
                return;
            }
        }
        if (auto&& pWatcher = SR_UTILS_NS::ResourceManager::Instance().StartWatch(path)) {
            pWatcher->SetCallBack([this](SR_UTILS_NS::FileWatcher*) {
                ++m_needReloadPage;
            });
            m_fileWatchers.emplace_back(pWatcher);
        }
    }

    void HTMLDrawerPass::InitRenderer() {
        if (!m_pPage) {
            return;
        }

        if (const auto pRenderer = m_pPage->GetContainer().PolymorphicCast<HTMLRenderContainer>()) {
            pRenderer->Init();
        }

        const SR_MATH_NS::UVector2 size = m_camera ? m_camera->GetSize() : GetContext()->GetWindowSize();

        m_pPage->GetDocument()->render(static_cast<int32_t>(size.x));
    }
}
