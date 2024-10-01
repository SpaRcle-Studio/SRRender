//
// Created by Monika on 14.08.2024.
//

#include <Graphics/Render/HTMLRenderer.h>
#include <Graphics/Memory/UBOManager.h>
#include <Graphics/Memory/DescriptorManager.h>
#include <Graphics/Types/Shader.h>

#include <Utils/Profile/TracyContext.h>

namespace SR_GRAPH_NS {
    HTMLDrawableElement::~HTMLDrawableElement() {
        SetShader(nullptr);
        SetTexture(nullptr);

        auto&& uboManager = SR_GRAPH_NS::Memory::UBOManager::Instance();
        auto&& descriptorManager = SR_GRAPH_NS::DescriptorManager::Instance();

        if (m_virtualUBO != SR_ID_INVALID && !uboManager.FreeUBO(&m_virtualUBO)) {
            SR_ERROR("HTMLDrawableElement::~HTMLDrawableElement() : failed to free virtual uniform buffer object!");
        }

        if (m_virtualDescriptor != SR_ID_INVALID) {
            descriptorManager.FreeDescriptorSet(&m_virtualDescriptor);
        }
    }

    void HTMLDrawableElement::SetShader(SR_GTYPES_NS::Shader::Ptr pShader) {
        m_dirtyMaterial |= m_pShader != pShader;
        m_pShader = pShader;
    }

    void HTMLDrawableElement::SetTexture(SR_GTYPES_NS::Texture::Ptr pTexture) {
        if (m_pTexture == pTexture) {
            return;
        }
        if (m_pTexture) {
            m_pTexture->RemoveUsePoint();
        }
        if ((m_pTexture = pTexture)) {
            pTexture->AddUsePoint();
        }
        m_dirtyMaterial = true;
    }

    void HTMLDrawableElement::Draw() {
        SR_TRACY_ZONE;

        auto&& uboManager = SR_GRAPH_NS::Memory::UBOManager::Instance();
        auto&& descriptorManager = SR_GRAPH_NS::DescriptorManager::Instance();

        if (m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
            m_virtualUBO = uboManager.AllocateUBO(m_virtualUBO);
            if (m_virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
                return;
            }

            m_virtualDescriptor = descriptorManager.AllocateDescriptorSet(m_virtualDescriptor);
        }

        uboManager.BindUBO(m_virtualUBO);

        const auto result = descriptorManager.Bind(m_virtualDescriptor);

        if (m_pipeline->GetCurrentBuildIteration() == 0) {
            if (result == DescriptorManager::BindResult::Duplicated || m_dirtyMaterial) SR_UNLIKELY_ATTRIBUTE {
                if (m_pTexture) {
                    m_pShader->SetSampler2D("image"_atom, m_pTexture);
                    m_pShader->FlushSamplers();
                }
                descriptorManager.Flush();
            }
            m_pipeline->GetCurrentShader()->FlushConstants();
        }

        if (result != DescriptorManager::BindResult::Failed) SR_UNLIKELY_ATTRIBUTE {
            m_pipeline->Draw(4);
        }

        m_dirtyMaterial = false;
    }

    void HTMLDrawableElement::Update(HTMLRendererUpdateContext& context) {
        if (m_virtualUBO == SR_ID_INVALID) SR_UNLIKELY_ATTRIBUTE {
            SRHalt("HTMLDrawableElement::Update() : virtual UBO is invalid!");
            return;
        }

        m_pipeline->SetCurrentShader(m_pShader);

        auto&& pNode = m_pPage->GetNodeById(m_nodeId);
        auto&& style = pNode->GetStyle();

        const SR_MATH_NS::FVector2 parentSize = context.size;

        context.size = SR_MATH_NS::FVector2(
            style.width.IsDefault() ? context.size.x : style.width.CalculateValue(context.size.x),
            style.height.IsDefault() ? context.size.y : style.height.CalculateValue(context.size.y)
        );

        auto&& uboManager = SR_GRAPH_NS::Memory::UBOManager::Instance();
        if (uboManager.BindNoDublicateUBO(m_virtualUBO) == Memory::UBOManager::BindResult::Success) SR_UNLIKELY_ATTRIBUTE {
            m_pShader->SetVec2("size"_atom_hash_cexpr, context.size / context.resolution);

            SR_MATH_NS::FVector2 position;

            if (style.position == SR_UTILS_NS::Web::CSSPosition::Relative) {
                //position.x = (position.x - 0.5f) * 2.f;
                //position.y = (position.y - 0.5f) * 2.f;
            }
            else if (style.position == SR_UTILS_NS::Web::CSSPosition::Absolute) {
                //position += context.size / 2.f;
                //position = (position / context.size) * 2.f - SR_MATH_NS::FVector2(1.f);

                /// установить начало координат в левом верхнем углу. на данный момент начало координат в центре
                position = SR_MATH_NS::FVector2(-1, 1);

                position.x += context.size.x / context.resolution.x;
                position.y -= context.size.y / context.resolution.y;
            }

            position.x += style.marginLeft.CalculateValue(parentSize.x) / context.size.x;
            position.y -= style.marginTop.CalculateValue(parentSize.x) / context.size.y;

            /// convert to shader screen space [-1, 1]
            m_pShader->SetVec2("position"_atom_hash_cexpr, position);

            SR_MAYBE_UNUSED_VAR m_pShader->Flush();
        }
    }

    /// ----------------------------------------------------------------------------------------------------------------

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
                continue;
            }
        }

        HTMLRendererUpdateContext context;
        context.size = m_pPage->GetSize().Cast<float_t>();
        context.resolution = m_pPage->GetSize().Cast<float_t>();
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
        if (!SRVerify(pDrawableElement)) {
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

    void HTMLRenderer::UpdateNode(const SR_UTILS_NS::Web::HTMLNode* pNode, HTMLRendererUpdateContext& context) {
        SR_TRACY_ZONE;

        if (!SRVerify(pNode)) {
            return;
        }

        auto&& pDrawableElement = static_cast<HTMLDrawableElement*>(pNode->GetUserData());
        if (SRVerify(pDrawableElement)) {
            pDrawableElement->Update(context);
        }

        for (const auto& childId : pNode->GetChildren()) {
            HTMLRendererUpdateContext parentContext = context;
            auto&& pChild = m_pPage->GetNodeById(childId);
            UpdateNode(pChild, parentContext);
        }
    }
}
