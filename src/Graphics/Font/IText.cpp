//
// Created by Monika on 20.06.2024.
//

#include <Graphics/Font/Font.h>
#include <Graphics/Font/IText.h>
#include <Graphics/Font/TextBuilder.h>

#include <Utils/Localization/Encoding.h>

namespace SR_GTYPES_NS {
    IText::IText()
        : Super(MeshType::Text)
    { }

    IText::~IText() {
        SetFont(nullptr);
    }

    void IText::SetUsePreprocessor(bool enabled) {
        m_preprocessor = enabled;
        OnTextDirty();
    }

    bool IText::Calculate() {
        if (IsCalculated()) {
            return true;
        }

        if (m_hasErrors || !IsCalculatable()) {
            return false;
        }

        if (!BuildAtlas()) {
            SR_ERROR("Text::Calculate() : failed to build atlas!");
            return false;
        }

        return Super::Calculate();
    }

    void IText::FreeVideoMemory() {
        SetFont(nullptr);

        if (m_id != SR_ID_INVALID) {
            SRVerifyFalse(!m_pipeline->FreeTexture(&m_id));
        }

        Super::FreeVideoMemory();
    }

    void IText::OnTextDirty() {
        m_isCalculated = false;
        if (auto&& pRenderScene = GetTextRenderScene()) {
            pRenderScene->SetDirty();
        }
    }

    bool IText::BuildAtlas() {
        if (!m_font) {
            SR_ERROR("Text::BuildAtlas() : missing font!");
            return false;
        }

        if (m_id != SR_ID_INVALID) {
            SRVerifyFalse(!m_pipeline->FreeTexture(&m_id));
        }

        TextBuilder textBuilder(m_font);
        textBuilder.SetKerning(m_kerning);
        textBuilder.SetDebug(m_debug);
        //textBuilder.SetCharSize(m_fontSize);

        if (!textBuilder.Build(m_text)) {
            return false;
        }

        m_atlasSize.x = textBuilder.GetWidth();
        m_atlasSize.y = textBuilder.GetHeight();

        SR_GRAPH_NS::SRTextureCreateInfo textureCreateInfo;

        textureCreateInfo.pData = textBuilder.GetData();
        textureCreateInfo.format = textBuilder.GetColorFormat();
        textureCreateInfo.width = m_atlasSize.x;
        textureCreateInfo.height = m_atlasSize.y;
        textureCreateInfo.compression = TextureCompression::None;
        textureCreateInfo.filter = TextureFilter::NEAREST;
        textureCreateInfo.mipLevels = 1;
        textureCreateInfo.cpuUsage = false;
        textureCreateInfo.alpha = true;

        EVK_PUSH_LOG_LEVEL(EvoVulkan::Tools::LogLevel::ErrorsOnly);

        m_id = m_pipeline->AllocateTexture(textureCreateInfo);

        EVK_POP_LOG_LEVEL();

        if (m_id == SR_ID_INVALID) {
            SR_ERROR("Text::BuildAtlas() : failed to build the font atlas!");
            return false;
        }

        return true;
    }

    void IText::UseMaterial() {
        Super::UseMaterial();
        UseModelMatrix();
    }

    void IText::UseModelMatrix() {
        GetRenderContext()->GetCurrentShader()->SetMat4(SHADER_MODEL_MATRIX, GetMatrix());
        GetRenderContext()->GetCurrentShader()->SetFloat(SHADER_TEXT_RECT_X, 0.f);
        GetRenderContext()->GetCurrentShader()->SetFloat(SHADER_TEXT_RECT_Y, 0.f);
        GetRenderContext()->GetCurrentShader()->SetFloat(SHADER_TEXT_RECT_WIDTH, static_cast<float_t>(m_atlasSize.x) / 100.f);
        GetRenderContext()->GetCurrentShader()->SetFloat(SHADER_TEXT_RECT_HEIGHT, static_cast<float_t>(m_atlasSize.y) / 100.f);

        Super::UseModelMatrix();
    }

    void IText::UseSamplers() {
        GetRenderContext()->GetCurrentShader()->SetSampler2D(SHADER_TEXT_ATLAS_TEXTURE, m_id);
        Mesh::UseSamplers();
    }

    bool IText::IsFlatMesh() const noexcept {
        return !m_is3D;
    }

    void IText::SetFont(Font* pFont) {
        if (pFont == m_font) {
            return;
        }

        if (m_font) {
            m_font->RemoveUsePoint();
        }

        if ((m_font = pFont)) {
            m_font->AddUsePoint();
        }

        OnTextDirty();
    }

    void IText::SetFontSize(const SR_MATH_NS::UVector2& size) {
        m_fontSize = size;
        OnTextDirty();
    }

    void IText::SetUseLocalization(bool enabled) {
        m_localization = enabled;
        OnTextDirty();
    }

    void IText::SetText(const std::string& text) {
        auto&& newText = SR_UTILS_NS::Localization::UtfToUtf<char32_t, char>(text);
        if (m_text == newText) {
            return;
        }
        m_text = std::move(newText);
        OnTextDirty();
    }

    void IText::SetText(const std::u16string& text) {
        auto&& newText = SR_UTILS_NS::Localization::UtfToUtf<char32_t, char16_t>(text);
        if (m_text == newText) {
            return;
        }
        m_text = std::move(newText);
        OnTextDirty();
    }

    void IText::SetText(const std::u32string& text) {
        if (m_text == text) {
            return;
        }
        m_text = text;
        OnTextDirty();
    }

    bool IText::IsCalculatable() const {
        return Super::IsCalculatable() && !m_text.empty() && m_font;
    }

    void IText::SetKerning(bool enabled) {
        m_kerning = enabled;
        OnTextDirty();
    }

    void IText::SetDebug(bool enabled) {
        m_debug = enabled;
        OnTextDirty();
    }

    void IText::SetFont(const SR_UTILS_NS::Path& path) {
        SetFont(SR_GTYPES_NS::Font::Load(path));
    }
}
