//
// Created by Monika on 29.04.2026.
//

#include <Graphics/Font/TextMesh.h>
#include <Graphics/Types/Shader.h>
#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/Font/TextMeshDetails.h>
#include <Graphics/Memory/DescriptorManager.h>

#include <Utils/ECS/TransformRect.h>
#include <Utils/Events/Broadcaster.h>

#include <Codegen/TextMesh.generated.hpp>

namespace SR_GTYPES_NS {
    SR_UTILS_NS::VertexLayoutDescriptionsRef TextMesh::GetShaderVertexLayoutDescriptions() const noexcept {
        static const auto description = SR_UTILS_NS::VertexLayoutDescription()
            .AddAttribute(SR_UTILS_NS::VertexAttribute::Position0, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
            .AddAttribute(SR_UTILS_NS::VertexAttribute::Position1, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
            .AddAttribute(SR_UTILS_NS::VertexAttribute::UV1, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
            .AddAttribute(SR_UTILS_NS::VertexAttribute::UV2, SR_UTILS_NS::VertexAttributeFormat::Float32, 2)
            .AddAttribute(SR_UTILS_NS::VertexAttribute::Color0, SR_UTILS_NS::VertexAttributeFormat::Float32, 4)
            .AddAttribute(SR_UTILS_NS::VertexAttribute::Custom0, SR_UTILS_NS::VertexAttributeFormat::Float32, 4)
            .AddAttribute(SR_UTILS_NS::VertexAttribute::Custom1, SR_UTILS_NS::VertexAttributeFormat::Float32, 4)
            .SetInstanced(true);
        return description;
    }

    void TextMesh::Draw() {
        if (!Calculate()) {
            return;
        }
        for (uint32_t i = 0; i < m_batches.size(); ++i) {
            auto&& batch = m_batches[i];
            if (batch.count == 0 || batch.vbo == SR_ID_INVALID) {
                continue;
            }
            m_drawBatch = i;
            GetPipeline()->BindVBO(batch.vbo, 0, VertexInputRate::Instance);
            GetPipeline()->SetDrawInstancesCount(batch.count);
            batch.dirty |= m_dirtyMaterial;
            DrawRenderObject(this, 6, m_virtualUBO, batch.descriptor, batch.dirty, m_hasErrors);
        }
        m_dirtyMaterial = false;
        GetPipeline()->ResetDrawInstancesCount();
    }

    void TextMesh::UseMaterial(Shader& shader) {
        UIRenderComponent::UseMaterial(shader);
        UseModelMatrix(shader);
        static const SR_UTILS_NS::StringAtom textVisible = "textVisible";
        static const SR_UTILS_NS::StringAtom textEffect = "textEffect";
        shader.SetFloat(textVisible, static_cast<float_t>(m_visibleGraphemes));
        shader.SetVec4(textEffect, m_characterEffect);
    }

    void TextMesh::UseModelMatrix(Shader& shader) {
        UIRenderComponent::UseModelMatrix(shader);
        shader.SetInt(SHADER_SPRITE_MODE, 0);
        if (auto&& pRect = GetTransformAs<SR_UTILS_NS::TransformRect>()) {
            shader.SetMat4(SHADER_MODEL_MATRIX, pRect->GetMatrix());
            shader.SetVec4(SHADER_NDC_RECT, pRect->GetLayoutRect().vec4);
        }
    }

    void TextMesh::UseSamplers(Shader& shader) {
        UIRenderComponent::UseSamplers(shader);
        if (m_drawBatch < m_batches.size()) {
            auto&& batch = m_batches[m_drawBatch];
            shader.SetSampler2D("diffuse", batch.key == std::numeric_limits<uint64_t>::max()
                ? shader.GetRenderContext()->GetNoneTexture() : batch.texture.GetResource());
        }
    }

    TextStyle TextMesh::ResolveStyle(uint32_t grapheme) const {
        TextStyle result;
        for (auto&& style : m_styles) {
            if (grapheme >= style.begin && grapheme < style.end) {
                result.bold |= style.bold;
                result.italic |= style.italic;
                result.underline |= style.underline;
                result.strike |= style.strike;
                result.relativeSize *= style.relativeSize;
                result.color = result.color * style.color;
            }
        }
        return result;
    }

    bool TextMesh::BuildGlyphs(SR_UTILS_NS::Vector<PositionedGlyph>& glyphs, SR_UTILS_NS::Vector<float_t>& scales) {
        SR_TRACY_ZONE;
        if (!m_onFontReloadedSubscription.IsValid()) {
            m_onFontReloadedSubscription = SR_UTILS_NS::Broadcaster::Instance().Subscribe(SR_UTILS_NS::Events::EVENT_ON_FONT_RELOADED_ID, [this](auto&&) {
                OnTextDirty();
            });
        }
        if (!UpdateGraphemes()) {
            return false;
        }
        auto pFont = m_font.GetResource();
        if (!pFont || !pFont->BuildText(m_text, m_fontSize, glyphs)) {
            SR_ERROR("TextMesh::BuildGlyphs() : unavailable font {}", m_font.GetId());
            return false;
        }
        m_glyphGraphemes.clear();
        scales.clear();
        uint32_t source = 0;
        for (auto&& glyph : glyphs) {
            while (source < m_codepoints.size() && m_codepoints[source].codepoint != glyph.codepoint.codepoint) {
                ++source;
            }
            const uint32_t grapheme = source < m_codepoints.size() ? m_codepoints[source++].grapheme : m_graphemeCount;
            m_glyphGraphemes.emplace_back(grapheme);
            scales.emplace_back(ResolveStyle(grapheme).relativeSize);
        }
        return true;
    }

    bool TextMesh::Calculate() {
        SR_TRACY_ZONE;
        SR_MATH_NS::FVector2 layoutSize;
        if (auto&& pRect = GetTransformAs<SR_UTILS_NS::TransformRect>()) {
            layoutSize = pRect->GetLayoutRect().Size();
        }
        if (layoutSize != m_calculatedLayoutSize) {
            m_isCalculated = false;
        }
        if (m_isCalculated) {
            return true;
        }
        if (!m_onFontReloadedSubscription.IsValid()) {
            m_onFontReloadedSubscription = SR_UTILS_NS::Broadcaster::Instance().Subscribe(SR_UTILS_NS::Events::EVENT_ON_FONT_RELOADED_ID, [this](auto&&) {
                OnTextDirty();
            });
        }

        struct TempCache {
            SR_UTILS_NS::Vector<PositionedGlyph> glyphs;
            SR_UTILS_NS::Vector<float_t> scales;
            SR_UTILS_NS::Vector<TextMeshDetails::GlyphPlacement> placements;
        };
        static SR_THREAD_LOCAL TempCache cache;
        if (!BuildGlyphs(cache.glyphs, cache.scales)) {
            return false;
        }
        for (auto&& batch : m_batches) {
            batch.count = 0;
        }
        m_instancesCount = 0;
        TextMeshDetails::GlyphPlacementContext context;
        context.fontSize = m_fontSize;
        context.kerning = m_kerning;
        context.wordWrap = m_wordWrap;
        context.pScales = &cache.scales;
        context.pGraphemes = &m_glyphGraphemes;
        context.horizontalAlignment = m_horizontalAlignment;
        context.verticalAlignment = m_verticalAlignment;
        context.layoutSize = layoutSize;
        TextMeshDetails::CalculateGlyphPlacements(*m_font.GetResource(), cache.glyphs, context, cache.placements);
        const auto layout = GetShaderVertexLayoutDescriptions().GetLayout(0);
        SetVertexLayoutDescription(layout);

        uint32_t visibleGlyphIndex = 0;
        for (auto&& placement : cache.placements) {
            auto&& glyph = cache.glyphs[placement.glyphIndex];
            const auto grapheme = m_glyphGraphemes[placement.glyphIndex];
            const auto style = ResolveStyle(grapheme);
            const bool solidOnly = placement.size.x <= 0.f || placement.size.y <= 0.f;
            if (solidOnly && !style.underline && !style.strike) {
                continue;
            }
            const uint64_t key = solidOnly ? std::numeric_limits<uint64_t>::max() : (static_cast<uint64_t>(glyph.metrics.fontId) << 32
                | static_cast<uint64_t>(glyph.renderType) << 16 | glyph.atlas.page);
            uint32_t batchIndex = 0;
            while (batchIndex < m_batches.size() && m_batches[batchIndex].key != key) {
                ++batchIndex;
            }
            if (batchIndex == m_batches.size()) {
                auto&& batch = m_batches.emplace_back();
                batch.key = key;
                if (!solidOnly) {
                    char path[96];
                    std::snprintf(path, sizeof(path), "FontAtlas/%u/%s/%u", static_cast<uint32_t>(glyph.metrics.fontId),
                        SR_UTILS_NS::EnumReflector::ToStringAtom(glyph.renderType).c_str(), static_cast<uint32_t>(glyph.atlas.page));
                    batch.texture.SetResource(path);
                    if (auto&& pTexture = batch.texture.GetResource()) {
                        batch.textureReloadSubscription = pTexture->Subscribe(SR_UTILS_NS::IResource::RELOAD_DONE_EVENT, [this](auto&&) {
                            MarkMaterialDirty();
                        });
                    }
                }
            }
            auto&& batch = m_batches[batchIndex];
            if (batch.count == 0) {
                batch.buffer.SetLayout(layout);
                batch.buffer.Allocate(cache.placements.size() * 3);
            }
            const float_t scale = m_fontSize * style.relativeSize / std::max(1.f, m_font.GetResource()->GetSamplingPointSize());
            const auto addQuad = [&](const SR_MATH_NS::FVector2& position, const SR_MATH_NS::FVector2& size, float_t kind) {
                const uint32_t index = batch.count++;
                glyph.AddInstance(index, position, size, batch.buffer);
                const SR_MATH_NS::FVector4 metadata(static_cast<float_t>(grapheme), style.bold ? 1.f : 0.f, style.italic ? 1.f : 0.f, kind);
                batch.buffer.SetVertex(index, SR_UTILS_NS::VertexAttribute::Color0, &style.color);
                batch.buffer.SetVertex(index, SR_UTILS_NS::VertexAttribute::Custom0, &metadata);
                const SR_MATH_NS::FVector4 effectIndex(static_cast<float_t>(visibleGlyphIndex), glyph.metrics.sdfRange,
                    glyph.renderType == GlyphRenderType::MTSDF ? 1.f : 0.f, scale);
                batch.buffer.SetVertex(index, SR_UTILS_NS::VertexAttribute::Custom1, &effectIndex);
            };
            const float_t kind = glyph.renderType == GlyphRenderType::ColorBitmap ? 3.f
                : glyph.renderType == GlyphRenderType::SDF ? 2.f : glyph.renderType == GlyphRenderType::Bitmap ? 4.f : 0.f;
            if (placement.size.x > 0.f && placement.size.y > 0.f) {
                addQuad(placement.pos, placement.size, kind);
            }
            const float_t advance = glyph.metrics.advance * scale;
            const float_t baseline = placement.pos.y - glyph.metrics.bearingY * scale + placement.size.y;
            if (style.underline) {
                addQuad({ placement.pos.x - glyph.metrics.bearingX * scale, baseline - m_fontSize * 0.12f },
                    { advance, std::max(1.f, m_fontSize * style.relativeSize * 0.06f) }, 1.f);
            }
            if (style.strike) {
                addQuad({ placement.pos.x - glyph.metrics.bearingX * scale, baseline + m_fontSize * style.relativeSize * 0.3f },
                    { advance, std::max(1.f, m_fontSize * style.relativeSize * 0.06f) }, 1.f);
            }
            if (placement.size.x > 0.f && placement.size.y > 0.f) {
                ++visibleGlyphIndex;
            }
        }

        for (auto&& batch : m_batches) {
            if (batch.count == 0) {
                continue;
            }
            const uint64_t size = batch.count * layout.GetStride();
            if (batch.vbo == SR_ID_INVALID || !GetPipeline()->UpdateVBO(batch.vbo, batch.buffer.GetRawData(), size)) {
                if (batch.vbo != SR_ID_INVALID) {
                    GetPipeline()->FreeVBO(&batch.vbo);
                }
                batch.vbo = GetPipeline()->AllocateVBO(SR_ID_INVALID, size, batch.buffer.GetRawData());
            }
            if (batch.vbo == SR_ID_INVALID) {
                SR_ERROR("TextMesh::Calculate() : failed VBO allocation");
                return false;
            }
            batch.dirty = true;
            m_instancesCount += batch.count;
        }
        m_isCalculated = true;
        m_calculatedLayoutSize = layoutSize;
        return true;
    }

    void TextMesh::CalculateLayoutInput(bool horizontal) {
        SR_TRACY_ZONE;
        if (horizontal && !m_metricsDirty) {
            return;
        }
        SR_UTILS_NS::Vector<PositionedGlyph> glyphs;
        SR_UTILS_NS::Vector<float_t> scales;
        SR_UTILS_NS::Vector<TextMeshDetails::GlyphPlacement> placements;
        if (!BuildGlyphs(glyphs, scales)) {
            m_preferredSize = {};
            m_minWidth = 0.f;
            return;
        }
        TextMeshDetails::GlyphPlacementContext context;
        context.fontSize = m_fontSize;
        context.kerning = m_kerning;
        context.pScales = &scales;
        context.pGraphemes = &m_glyphGraphemes;
        SR_MATH_NS::FVector2 preferred;
        context.pPreferredSize = &preferred;
        context.pMinWidth = &m_minWidth;
        if (!horizontal && m_wordWrap) {
            if (auto&& pRect = GetTransformAs<SR_UTILS_NS::TransformRect>()) {
                context.layoutSize.x = pRect->GetLayoutRect().Size().x;
                context.wordWrap = true;
            }
        }
        TextMeshDetails::CalculateGlyphPlacements(*m_font.GetResource(), glyphs, context, placements);
        if (horizontal) {
            m_preferredSize = preferred;
        }
        else {
            m_preferredSize.y = preferred.y;
            m_metricsDirty = false;
        }
    }

    SR_MATH_NS::FRect TextMesh::GetTextBounds() {
        SR_TRACY_ZONE;
        SR_UTILS_NS::Vector<PositionedGlyph> glyphs;
        SR_UTILS_NS::Vector<float_t> scales;
        SR_UTILS_NS::Vector<TextMeshDetails::GlyphPlacement> placements;
        if (!BuildGlyphs(glyphs, scales)) {
            return {};
        }
        TextMeshDetails::GlyphPlacementContext context;
        context.fontSize = m_fontSize;
        context.kerning = m_kerning;
        context.wordWrap = m_wordWrap;
        context.pScales = &scales;
        context.pGraphemes = &m_glyphGraphemes;
        context.horizontalAlignment = m_horizontalAlignment;
        context.verticalAlignment = m_verticalAlignment;
        if (auto&& pRect = GetTransformAs<SR_UTILS_NS::TransformRect>()) {
            context.layoutSize = pRect->GetLayoutRect().Size();
        }
        TextMeshDetails::CalculateGlyphPlacements(*m_font.GetResource(), glyphs, context, placements);
        TextMeshDetails::Bounds bounds;
        for (auto&& placement : placements) {
            if (placement.size.x > 0.f && placement.size.y > 0.f) {
                bounds.IncludeQuad(placement.pos.x, placement.pos.y, placement.size);
            }
        }
        return bounds.IsValid() ? SR_MATH_NS::FRect(bounds.minX, bounds.minY, bounds.maxX - bounds.minX, bounds.maxY - bounds.minY) : SR_MATH_NS::FRect();
    }

    void TextMesh::OnDestroy() {
        m_onFontReloadedSubscription.Reset();
        UIRenderComponent::OnDestroy();
    }

    void TextMesh::FreeVideoMemory() {
        for (auto&& batch : m_batches) {
            if (auto&& pPipeline = TryGetPipeline()) {
                if (batch.vbo != SR_ID_INVALID) {
                    pPipeline->FreeVBO(&batch.vbo);
                }
                pPipeline->GetDescriptorManager().TryFreeDescriptorSet(&batch.descriptor);
            }
        }
        m_batches.clear();
        m_isCalculated = false;
        m_instancesCount = 0;
        UIRenderComponent::FreeVideoMemory();
    }

    void TextMesh::SetText(SR_UTILS_NS::StringView text) {
        if (m_text == text && m_styles.empty()) {
            SetVisibleGraphemeCount(std::numeric_limits<uint32_t>::max());
            return;
        }
        m_text = text;
        m_styles.clear();
        m_visibleGraphemes = std::numeric_limits<uint32_t>::max();
        OnTextDirty();
    }

    void TextMesh::SetVisibleGraphemeCount(uint32_t count) {
        if (m_visibleGraphemes != count) {
            m_visibleGraphemes = count;
            MarkUniformsDirty();
        }
    }

    uint32_t TextMesh::GetGraphemeCount() {
        return UpdateGraphemes() ? m_graphemeCount : 0;
    }

    bool TextMesh::UpdateGraphemes() {
        if (m_graphemesDirty) {
            m_validUTF8 = SR_UTILS_NS::Localization::DecodeGraphemes(m_text, m_codepoints, m_graphemeCount);
            m_graphemesDirty = false;
        }
        return m_validUTF8;
    }

    void TextMesh::AddStyle(const TextStyle& style) {
        if (style.begin >= style.end || !std::isfinite(style.relativeSize) || style.relativeSize <= 0.f) {
            SR_ERROR("TextMesh::AddStyle() : invalid range or relative size");
            return;
        }
        m_styles.emplace_back(style);
        OnTextDirty();
    }

    void TextMesh::ClearStyles() {
        m_styles.clear();
        OnTextDirty();
    }

    void TextMesh::SetCharacterEffect(float_t time, float_t waveAmplitude, float_t shakeAmplitude, float_t frequency) {
        m_characterEffect = { time, waveAmplitude, shakeAmplitude, frequency };
        MarkUniformsDirty();
    }

    SR_UTILS_NS::Optional<int32_t> TextMesh::GetVBO() const {
        const_cast<TextMesh*>(this)->Calculate();
        for (auto&& batch : m_batches) {
            if (batch.vbo != SR_ID_INVALID) {
                return batch.vbo;
            }
        }
        return {};
    }

    bool TextMesh::Bind() {
        if (!Calculate()) {
            return false;
        }
        const auto vbo = GetVBO();
        if (!vbo) {
            return false;
        }
        GetPipeline()->BindVBO(vbo.value(), 0, VertexInputRate::Instance);
        return true;
    }

    void TextMesh::OnMatrixDirty() {
        m_isCalculated = false;
        if (auto&& pScene = TryGetRenderScene()) {
            pScene->SetDirty();
        }
        UIRenderComponent::OnMatrixDirty();
    }

    void TextMesh::OnTextDirty() {
        ++m_layoutInputRevision;
        m_graphemesDirty = true;
        m_isCalculated = false;
        m_metricsDirty = true;
        m_dirtyMaterial = true;
        SetLayoutDirty();
        if (auto&& pScene = TryGetRenderScene()) {
            pScene->SetDirty();
        }
    }
}
