//
// Created by Monika on 29.04.2026.
//

#include <Graphics/Font/TextMesh.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/ECS/TransformUtils.h>
#include <Utils/ECS/TransformRect.h>
#include <Utils/Common/Vertices.h>

#include <Codegen/TextMesh.generated.hpp>

namespace SR_GTYPES_NS {
    const SR_UTILS_NS::VertexLayoutDescription& TextMesh::GetShaderVertexLayoutDescription() const noexcept {
        static const auto description = SR_UTILS_NS::VertexLayoutDescription()
            .AddAttribute(SR_UTILS_NS::VertexAttribute::Position0, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
            .AddAttribute(SR_UTILS_NS::VertexAttribute::Position1, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
            .AddAttribute(SR_UTILS_NS::VertexAttribute::UV1, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
            .AddAttribute(SR_UTILS_NS::VertexAttribute::UV2, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
            .SetInstanced(true);
        return description;
    }

    void TextMesh::Draw() {
        Calculate();
        GetPipeline()->SetDrawInstancesCount(m_instancesCount);
        DrawRenderObject(this, 6, m_virtualUBO, m_virtualDescriptor, m_dirtyMaterial, m_hasErrors);
        GetPipeline()->ResetDrawInstancesCount();
    }

    void TextMesh::UseMaterial(Shader& shader) {
        Super::UseMaterial(shader);
        UseModelMatrix(shader);
    }

    void TextMesh::UseModelMatrix(Shader& shader) {
        Super::UseModelMatrix(shader);

        shader.SetInt(SHADER_SPRITE_MODE, static_cast<int32_t>(0));

        if (auto&& pTransformRect = SR_UTILS_NS::ExtractTransformAs<SR_UTILS_NS::TransformRect>(GetSceneObject().Get())) SR_LIKELY_ATTRIBUTE {
            SR_MATH_NS::FRect layout = pTransformRect->GetLayoutRect();
            shader.SetVec4(SHADER_NDC_RECT, layout.vec4);
            shader.SetMat4(SHADER_MODEL_MATRIX, pTransformRect->GetMatrix());
        }
    }

    void TextMesh::UseSamplers(Shader& shader) {
        Super::UseSamplers(shader);
        static const SR_UTILS_NS::StringAtom id = "diffuse";
        //shader.SetSampler2D(id, 0);
    }

    bool TextMesh::Calculate() {
        SR_TRACY_ZONE;

        if (m_isCalculated) {
            return true;
        }

        auto&& pFontAsset = m_font.GetResource();
        if (!pFontAsset) {
            SR_ERROR("TextMesh::Calculate() : failed to load font asset!");
            m_hasErrors = true;
            return false;
        }

        static SR_THREAD_LOCAL std::vector<PositionedGlyph> glyphs;
        pFontAsset->BuildText(m_text, m_fontSize, glyphs);
        m_instancesCount = static_cast<uint32_t>(glyphs.size());

        if (m_VBO != SR_ID_INVALID) {
            GetPipeline()->FreeVBO(&m_VBO);
        }

        if (glyphs.empty()) {
            return true;
        }

        static SR_THREAD_LOCAL SR_UTILS_NS::VertexDataBuffer buffer;
        buffer.SetLayout(GetShaderVertexLayoutDescription());
        buffer.Allocate(glyphs.size());

        SetVertexLayoutDescription(GetShaderVertexLayoutDescription());

        const float_t layoutScale = m_fontSize / std::max(1.f, pFontAsset->GetSamplingPointSize());

        SR_MATH_NS::FVector2 textPos = { 100.f, 400.f };

        float_t penX = 0.0f;

        float_t baselineY = textPos.y + pFontAsset->GetFontAscender() * layoutScale;
        const float_t ascender  = pFontAsset->GetFontAscender() * layoutScale;
        const float_t descender = pFontAsset->GetFontDescender() * layoutScale;
        const float_t lineGap   = pFontAsset->GetFontLineGap() * layoutScale;
        const float_t lineHeight = (ascender - descender) + lineGap;

        for (auto&& glyph : glyphs) {
            if (glyph.codepoint.codepoint == '\n') {
                penX = 0.0f;
                baselineY -= lineHeight;
                continue;
            }

            const float_t x = textPos.x + penX + glyph.metrics.bearingX * layoutScale;
            const float_t y = baselineY - glyph.metrics.bearingY * layoutScale;

            const SR_MATH_NS::FVector2 size = glyph.metrics.size.CastToFloat() * layoutScale;

            glyph.AddInstance({x, y}, size, buffer);

            penX += glyph.metrics.advance * layoutScale;
        }

        m_VBO = GetPipeline()->AllocateVBO(buffer.GetDataSize(), buffer.GetRawData());
        if (m_VBO == SR_ID_INVALID) {
            SR_ERROR("TextMesh::Calculate() : failed to allocate VBO for text mesh!");
            m_hasErrors = true;
            return false;
        }

        m_isCalculated = true;
        return true;
    }

    void TextMesh::FreeVideoMemory() {
        Super::FreeVideoMemory();
        if (m_VBO != SR_ID_INVALID) {
            GetPipeline()->FreeVBO(&m_VBO);
        }
    }

    std::optional<int32_t> TextMesh::GetVBO() const {
        const_cast<TextMesh*>(this)->Calculate();
        return m_VBO != SR_ID_INVALID ? std::optional<int32_t>(m_VBO) : std::nullopt;
    }

    bool TextMesh::Bind() {
        Calculate();
        if (m_VBO == SR_ID_INVALID) {
            return false;
        }
        GetPipeline()->BindVBO(m_VBO);
        return true;
    }
}