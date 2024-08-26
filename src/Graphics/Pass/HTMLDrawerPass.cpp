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

    bool HTMLDrawerPass::Load(const SR_XML_NS::Node& passNode) {
        SR_TRACY_ZONE;
        auto&& path = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(passNode.GetAttribute("Path").ToString());
        m_pPage = SR_UTILS_NS::Web::HTMLParser::Instance().Parse(path);
        return BasePass::Load(passNode);
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

        if (m_pPage) {
            const SR_MATH_NS::UVector2 size = m_camera ? m_camera->GetSize() : GetContext()->GetWindowSize();

            m_pRenderer = HTMLRenderer::MakeShared(GetPassPipeline().Get(), m_pPage);
            m_pRenderer->SetScreenSize(size);
            m_pRenderer->Init();
        }

        return result;
    }

    void HTMLDrawerPass::DeInit() {
        if (m_pRenderer) {
            m_pRenderer->DeInit();
        }
        Super::DeInit();
    }

    void HTMLDrawerPass::OnResize(const SR_MATH_NS::UVector2& size) {
        if (m_pRenderer) {
            m_pRenderer->SetScreenSize(size);
        }
        Super::OnResize(size);
    }
}
