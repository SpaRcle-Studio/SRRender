//
// Created by Monika on 20.06.2024.
//

#include <Graphics/Font/Text.h>
#include <Graphics/Font/Font.h>
#include <Graphics/Font/TextBuilder.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Render/RenderScene.h>

#include <EvoVulkan/Tools/VulkanDebug.h>

#include <Utils/Localization/Encoding.h>
#include <Utils/ECS/SceneObject.h>
#include <Utils/ECS/TransformRect.h>
#include <Utils/FileSystem/PathDataAccessor.h>

#include <Codegen/Text.generated.hpp>

namespace SR_GTYPES_NS {
    Text::Text()
        : Super()
    { }

    Text::~Text() {
        SetFont(SR_GTYPES_NS::Font::Ptr());
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

    void Text::FreeVMemory() {
        SetFont(SR_GTYPES_NS::Font::Ptr());

        if (m_id != SR_ID_INVALID) {
            SRVerifyFalse(!m_pipeline->FreeTexture(&m_id));
        }

        Super::FreeVMemory();
    }

    void Text::OnTextDirty() {
        m_isCalculated = false;
        if (auto&& pRenderScene = TryGetRenderScene()) {
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

        TextBuilder textBuilder(m_font.Get());
        textBuilder.SetKerning(m_kerning);
        textBuilder.SetDebug(m_debug);
        textBuilder.SetFontSize(m_fontSize);
        textBuilder.SetInvertX(true);
        textBuilder.SetInvertY(true);

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
        textureCreateInfo.addressMode = AddressMode::ClampToEdge;
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

    void Text::UseMaterial(SR_GTYPES_NS::Shader& shader) {
        Super::UseMaterial(shader);
        UseModelMatrix(shader);
    }

    void Text::UseModelMatrix(SR_GTYPES_NS::Shader& shader) {
        shader.SetMat4(SHADER_MODEL_MATRIX, GetMatrix());

        if (auto&& pTransformRect = SR_UTILS_NS::ExtractTransformAs<SR_UTILS_NS::TransformRect>(GetSceneObject().Get())) SR_LIKELY_ATTRIBUTE {
            SR_MATH_NS::FRect layout = pTransformRect->GetLayoutRect();
            shader.SetVec4(SHADER_NDC_RECT, layout.vec4);
        }

        shader.SetVec2(SHADER_TEXT_ATLAS_SIZE, m_atlasSize.CastToFloat());

        Super::UseModelMatrix(shader);
    }

    void Text::UseSamplers(SR_GTYPES_NS::Shader& shader) {
        shader.SetSampler2D(SHADER_TEXT_ATLAS_TEXTURE, m_id);
        Mesh::UseSamplers(shader);
    }

    bool Text::IsFlatMesh() const noexcept {
        auto&& pTransform = GetTransform();
        return pTransform && pTransform->GetMeasurement() == SR_UTILS_NS::Measurement::Space2D;
    }

    void Text::SetFont(const SR_GTYPES_NS::Font::Ptr& pFont) {
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

    void Text::SetFontSize(const uint16_t& size) {
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
        if (path.empty()) {
            SetFont(SR_GTYPES_NS::Font::Ptr());
            return;
        }
        SetFont(SR_GTYPES_NS::Font::Load(path));
    }

    SR_UTILS_NS::Path Text::GetFontPath() const noexcept {
        if (m_font) {
            return m_font->GetResourcePath();
        }
        return SR_UTILS_NS::Path();
    }
}
