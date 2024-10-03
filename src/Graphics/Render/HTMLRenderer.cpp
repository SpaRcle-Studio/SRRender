//
// Created by Monika on 14.08.2024.
//

#include <Graphics/Render/HTMLRenderer.h>
#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Memory/DescriptorManager.h>
#include <Graphics/Types/Shader.h>

#include <Utils/Profile/TracyContext.h>

namespace SR_GRAPH_NS {
    HTMLRenderer::HTMLRenderer(Pipeline* pPipeline, SR_UTILS_NS::Web::HTMLPage::Ptr pPage)
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        , m_pPage(std::move(pPage))
        , m_pipeline(pPipeline)
    { }

    HTMLRenderer::~HTMLRenderer() {
        SRAssert2(m_drawableElements.empty(), "HTMLRenderer::~HTMLRenderer() : not all elements were deleted!");
        SRAssert2(m_shaders.empty(), "HTMLRenderer::~HTMLRenderer() : not all shaders were deleted!");
    }

    bool HTMLRenderer::Init() {
        if (!m_shaders.empty()) {
            SR_ERROR("HTMLRenderer::Init() : shaders are already initialized!");
            return false;
        }

        if (auto&& pShader = SR_GTYPES_NS::Shader::Load("Engine/Shaders/Web/html.srsl")) {
            pShader->AddUsePoint();
            m_shaders["common"] = pShader;
        }
        else {
            return false;
        }

        if (auto&& pShader = SR_GTYPES_NS::Shader::Load("Engine/Shaders/Web/html-image.srsl")) {
            pShader->AddUsePoint();
            m_shaders["image"] = pShader;
        }
        else {
            return false;
        }

        SRAssert2(m_drawableElements.empty(), "HTMLRenderer::Init() : drawable elements are not empty!");

        m_pPage->RemoveUserDataRecursively();
        PrepareNode(m_pPage->GetBody());

        return true;
    }

    void HTMLRenderer::DeInit() {
        for (auto&& pElement : m_drawableElements) {
            delete pElement;
        }
        m_drawableElements.clear();

        for (auto&& pShader: m_shaders | std::views::values) {
            pShader->RemoveUsePoint();
        }
        m_shaders.clear();
    }

    void HTMLRenderer::Draw() {
        SR_TRACY_ZONE;

        if (!m_pPage) {
            SRHalt("HTMLRenderer::Draw() : page is nullptr!");
            return;
        }

        m_pipeline->SetCurrentShader(nullptr);

        DrawNode(m_pPage->GetBody());

        if (auto&& pShader = m_pipeline->GetCurrentShader()) {
            pShader->UnUse();
        }
    }

    void HTMLRenderer::Update() {
        SR_TRACY_ZONE;

        if (!m_pPage) {
            SRHalt("HTMLRenderer::Update() : page is nullptr!");
            return;
        }

        for (auto&& pShader : m_shaders | std::views::values) {
            m_pipeline->SetCurrentShader(pShader);

            if (!pShader || !pShader->Ready() || !m_pCamera) SR_UNLIKELY_ATTRIBUTE {
                if (pShader && pShader->HasErrors()) {
                    SR_ERROR("HTMLDrawerPass::Update() : shader has errors! Shader: {}", pShader->GetResourcePath().ToStringView());
                    return;
                }
                continue;
            }

            if (pShader->BeginSharedUBO()) SR_LIKELY_ATTRIBUTE {
                pShader->SetMat4(SHADER_ORTHOGONAL_MATRIX, m_pCamera->GetOrthogonal());
                pShader->SetVec2(SHADER_RESOLUTION, m_pPage->GetSize().Cast<float_t>());

                const float_t aspect = m_pPage->GetSize().Aspect();
                const SR_MATH_NS::FVector2 aspectVec = aspect > 1.f ? SR_MATH_NS::FVector2(1.f / aspect, 1.f) : SR_MATH_NS::FVector2(1.f, aspect);
                pShader->SetVec2(SHADER_ASPECT, aspectVec);

                pShader->EndSharedUBO();
            }
            else {
                SR_ERROR("HTMLDrawerPass::Update() : failed to bind shared UBO for shader {}!", pShader->GetResourcePath().ToStringView());
                return;
            }
        }

        HTMLRendererUpdateContext context;
        context.size = m_pPage->GetSize().Cast<float_t>();
        context.resolution = context.size;
        UpdateNode(m_pPage->GetBody(), context);
    }

    void HTMLRenderer::SetScreenSize(const SR_MATH_NS::UVector2& size) {
        if (m_pPage) {
            m_pPage->SetSize(size);
        }
    }

    void HTMLRenderer::PrepareNode(SR_UTILS_NS::Web::HTMLNode* pNode) {
        SR_TRACY_ZONE;

        if (!SRVerify(pNode)) {
            return;
        }

        auto&& pDrawableElement = m_drawableElements.emplace_back(new HTMLDrawableElement());

        if (pNode->GetTag() == SR_UTILS_NS::Web::HTMLTag::Img) {
            std::string_view src = pNode->GetAttributeByName("src")->GetValue();
            if (auto&& pTexture = SR_GTYPES_NS::Texture::Load(src)) {
                pDrawableElement->SetTexture(pTexture);
                pDrawableElement->SetShader(m_shaders["image"]);
            }
            else {
                SR_ERROR("HTMLRenderer::PrepareNode() : failed to load texture {}!", src);
                pDrawableElement->SetShader(m_shaders["common"]);
            }
        }
        else {
            pDrawableElement->SetShader(m_shaders["common"]);
        }

        pDrawableElement->SetNodeId(pNode->GetId());
        pDrawableElement->SetPipeline(m_pipeline);
        pDrawableElement->SetPage(m_pPage.Get());

        pNode->SetUserData(pDrawableElement);

        for (const auto& childId : pNode->GetChildren()) {
            auto&& pChild = m_pPage->GetNodeById(childId);
            PrepareNode(pChild);
        }
    }

    void HTMLRenderer::DrawNode(const SR_UTILS_NS::Web::HTMLNode* pNode) {
        SR_TRACY_ZONE;

        if (!SRVerify(pNode)) {
            return;
        }

        auto&& pDrawableElement = static_cast<HTMLDrawableElement*>(pNode->GetUserData());
        if (!pDrawableElement) {
            return;
        }

        auto&& pShader = pDrawableElement->GetShader();
        if (pShader != m_pipeline->GetCurrentShader()) {
            if (m_pipeline->GetCurrentShader()) {
                m_pipeline->GetCurrentShader()->UnUse();
            }
            if (pShader->Use() == ShaderBindResult::Failed) {
                return;
            }
        }

        pDrawableElement->Draw();

        for (const auto& childId : pNode->GetChildren()) {
            auto&& pChild = m_pPage->GetNodeById(childId);
            DrawNode(pChild);
        }
    }

    HTMLRendererUpdateResult HTMLRenderer::UpdateNode(const SR_UTILS_NS::Web::HTMLNode* pNode, const HTMLRendererUpdateContext& parentContext) {
        SR_TRACY_ZONE;

        HTMLRendererUpdateResult result;
        if (!SRVerify(pNode)) {
            return result;
        }

        auto&& pDrawableElement = static_cast<HTMLDrawableElement*>(pNode->GetUserData());
        if (!pDrawableElement) {
            return result;
        }

        HTMLRendererUpdateContext context = parentContext;

        auto&& style = pDrawableElement->GetStyle();

        context.size.x = style.width.CalculateValue(parentContext.size.x);
        context.size.y = style.height.CalculateValue(parentContext.size.y);

        result = pDrawableElement->Update(context);

        if (pNode->GetChildren().empty()) {
            return result;
        }

        for (const auto& childId : pNode->GetChildren()) {
            auto&& pChild = m_pPage->GetNodeById(childId);
            UpdateNode(pChild, context);
        }

        return result;
    }
}
