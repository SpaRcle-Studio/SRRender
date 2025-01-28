//
// Created by Monika on 20.06.2024.
//

#include <Graphics/Font/Font.h>
#include <Graphics/Font/IText.h>
#include <Graphics/Font/TextBuilder.h>

#include <Utils/Localization/Encoding.h>
#include <Utils/ECS/SceneObject.h>
#include <Utils/ECS/Transform2D.h>

#include <Codegen/IText.generated.hpp>

namespace SR_GTYPES_NS {
    Text::Text()
        : Super()
    {
        m_entityMessages.AddStandardProperty<SR_MATH_NS::UVector2>("Atlas size", &m_atlasSize)
            .SetDontSave()
            .SetReadOnly();
    }

    Text::~Text() {
        SetFont(nullptr);
    }

    bool Text::InitializeEntity() noexcept {
        GetComponentProperties().AddStandardProperty("Is 3D", &m_is3D)
            .SetSetter([this](void* pValue) { m_is3D = *static_cast<bool*>(pValue); ReRegisterMesh(); });

        GetComponentProperties().AddStandardProperty("Use localization", &m_localization)
            .SetSetter([this](void* pValue) { SetUseLocalization(*static_cast<bool*>(pValue)); });

        GetComponentProperties().AddStandardProperty("Use preprocessor", &m_preprocessor)
            .SetSetter([this](void* pValue) { SetUsePreprocessor(*static_cast<bool*>(pValue)); });

        GetComponentProperties().AddStandardProperty("Use kerning", &m_kerning)
            .SetSetter([this](void* pValue) { SetKerning(*static_cast<bool*>(pValue)); });

        GetComponentProperties().AddStandardProperty("Debug", &m_debug)
            .SetSetter([this](void* pValue) { SetDebug(*static_cast<bool*>(pValue)); });

        GetComponentProperties().AddStandardProperty("Font size", &m_fontSize)
            .SetDrag(1)
            .SetResetValue(512.f)
            .SetWidth(90.f);

        GetComponentProperties().AddStandardProperty("Text", &m_text)
            .SetSetter([this](auto&& text) { SetText(*static_cast<SR_HTYPES_NS::UnicodeString*>(text)); })
            .SetMultiline();

        GetComponentProperties().AddCustomProperty<SR_UTILS_NS::PathProperty>("Font")
            .AddFileFilter("Mesh", SR_GRAPH_NS::SR_SUPPORTED_FONT_FORMATS)
            .SetGetter([this]()-> SR_UTILS_NS::Path {
                return m_font ? m_font->GetResourcePath() : SR_UTILS_NS::Path();
            })
            .SetSetter([this](const SR_UTILS_NS::Path& path) {
                SetFont(path);
            });

        return Super::InitializeEntity();
    }

    int64_t Text::GetSortingPriority() const {
        if (auto&& pTransform = GetTransform()) {
            if (pTransform->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D) {
                return static_cast<SR_UTILS_NS::Transform2D*>(pTransform)->GetPriority();
            }
        }

        return -1;
    }

    bool Text::HasSortingPriority() const {
        if (auto&& pTransform = GetTransform()) {
            return pTransform->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D;
        }
        return false;
    }

    SR_UTILS_NS::StringAtom Text::GetMeshLayer() const {
        if (!m_sceneObject) {
            return SR_UTILS_NS::StringAtom();
        }

        return m_sceneObject->GetLayer();
    }

    const SR_MATH_NS::Matrix4x4& Text::GetMatrix() const {
        if (auto&& pTransform = GetTransform()) {
            return pTransform->GetMatrix();
        }

        return Super::GetMatrix();
    }

    void Text::SetUsePreprocessor(bool enabled) {
        m_preprocessor = enabled;
        OnTextDirty();
    }

    bool Text::Calculate() {
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

    void Text::FreeVideoMemory() {
        SetFont(nullptr);

        if (m_id != SR_ID_INVALID) {
            SRVerifyFalse(!m_pipeline->FreeTexture(&m_id));
        }

        Super::FreeVideoMemory();
    }

    void Text::OnTextDirty() {
        m_isCalculated = false;
        if (auto&& pRenderScene = GetTextRenderScene()) {
            pRenderScene->SetDirty();
        }
    }

    bool Text::BuildAtlas() {
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

    void Text::UseMaterial() {
        Super::UseMaterial();
        UseModelMatrix();
    }

    void Text::UseModelMatrix() {
        GetRenderContext()->GetCurrentShader()->SetMat4(SHADER_MODEL_MATRIX, GetMatrix());
        GetRenderContext()->GetCurrentShader()->SetFloat(SHADER_TEXT_RECT_X, 0.f);
        GetRenderContext()->GetCurrentShader()->SetFloat(SHADER_TEXT_RECT_Y, 0.f);
        GetRenderContext()->GetCurrentShader()->SetFloat(SHADER_TEXT_RECT_WIDTH, static_cast<float_t>(m_atlasSize.x) / 100.f);
        GetRenderContext()->GetCurrentShader()->SetFloat(SHADER_TEXT_RECT_HEIGHT, static_cast<float_t>(m_atlasSize.y) / 100.f);

        Super::UseModelMatrix();
    }

    void Text::UseSamplers() {
        GetRenderContext()->GetCurrentShader()->SetSampler2D(SHADER_TEXT_ATLAS_TEXTURE, m_id);
        Mesh::UseSamplers();
    }

    bool Text::IsFlatMesh() const noexcept {
        return !m_is3D;
    }

    void Text::SetFont(Font* pFont) {
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

    void Text::SetFontSize(const SR_MATH_NS::UVector2& size) {
        m_fontSize = size;
        OnTextDirty();
    }

    void Text::SetUseLocalization(bool enabled) {
        m_localization = enabled;
        OnTextDirty();
    }

    void Text::SetText(const std::string& text) {
        auto&& newText = SR_UTILS_NS::Localization::UtfToUtf<char32_t, char>(text);
        if (m_text == newText) {
            return;
        }
        m_text = std::move(newText);
        OnTextDirty();
    }

    void Text::SetText(const std::u16string& text) {
        auto&& newText = SR_UTILS_NS::Localization::UtfToUtf<char32_t, char16_t>(text);
        if (m_text == newText) {
            return;
        }
        m_text = std::move(newText);
        OnTextDirty();
    }

    void Text::SetText(const std::u32string& text) {
        if (m_text == text) {
            return;
        }
        m_text = text;
        OnTextDirty();
    }

    bool Text::IsCalculatable() const {
        return Super::IsCalculatable() && !m_text.empty() && m_font;
    }

    void Text::SetKerning(bool enabled) {
        m_kerning = enabled;
        OnTextDirty();
    }

    void Text::SetDebug(bool enabled) {
        m_debug = enabled;
        OnTextDirty();
    }

    void Text::SetFont(const SR_UTILS_NS::Path& path) {
        SetFont(SR_GTYPES_NS::Font::Load(path));
    }
}
