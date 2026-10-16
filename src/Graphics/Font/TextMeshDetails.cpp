//
// Created by Monika on 17.10.2026.
//

#include <Graphics/Font/TextMeshDetails.h>

namespace SR_GTYPES_NS::TextMeshDetails {
    void ApplyLineJustification(
        SR_UTILS_NS::Vector<GlyphPlacement>& placements,
        TextLine& line,
        float_t layoutWidth,
        float_t extraPerSpace
    ) {
        if (extraPerSpace <= 0.f || line.spacePlacementIndices.empty()) {
            return;
        }

        float_t cumulativeExtra = 0.f;
        size_t nextSpace = 0;

        for (size_t i = line.placementBegin; i < line.placementEnd; ++i) {
        if (nextSpace < line.spacePlacementIndices.size() && line.spacePlacementIndices[nextSpace] == i) {
        cumulativeExtra += extraPerSpace;
        ++nextSpace;
        }
        placements[i].pos.x += cumulativeExtra;
        }

        line.penWidth = layoutWidth;
    }

    bool CalculateGlyphPlacements(
        FontAsset& fontAsset,
        const SR_UTILS_NS::Vector<PositionedGlyph>& glyphs,
        GlyphPlacementContext context,
        SR_UTILS_NS::Vector<GlyphPlacement>& placements
    ) {
        const float_t layoutScale = context.fontSize / std::max(1.f, fontAsset.GetSamplingPointSize());

        const float_t ascender  = fontAsset.GetFontAscender() * layoutScale;
        const float_t descender = fontAsset.GetFontDescender() * layoutScale;
        const float_t lineGap   = fontAsset.GetFontLineGap() * layoutScale;
        const float_t lineHeight = (ascender - descender) + lineGap;
        const float_t capline = ascender * fontAsset.GetCapLineAscenderRatio();
        const float_t midline = (ascender - descender) * 0.5f;

        static SR_THREAD_LOCAL std::vector<TextMeshDetails::TextLine> lines;

        placements.clear();
        lines.clear();

        TextMeshDetails::TextLine currentLine;
        currentLine.baselineY = ascender;
        float_t penX = 0.f;
        std::optional<GlyphKey> prevCode;

        TextMeshDetails::Bounds bounds;

        auto finalizeLine = [&]() {
            currentLine.placementEnd = placements.size();
            if (currentLine.placementEnd > currentLine.placementBegin) {
                currentLine.penWidth = penX;
                lines.push_back(currentLine);
            }
            currentLine = {};
            currentLine.baselineY = lines.empty() ? ascender : lines.back().baselineY - lineHeight;
            currentLine.placementBegin = placements.size();
            penX = 0.f;
            prevCode.reset();
        };

        for (size_t glyphIndex = 0; glyphIndex < glyphs.size(); ++glyphIndex) {
            auto&& glyph = glyphs[glyphIndex];

            if (glyph.codepoint.codepoint == '\n') {
                finalizeLine();
                continue;
            }

            float_t kerning = 0.f;
            if (prevCode && context.kerning) {
                kerning = fontAsset.GetKerning(prevCode.value(), glyph.codepoint);
                kerning *= layoutScale;
            }

            const float_t cursorX = penX + kerning;
            const float_t x = cursorX + glyph.metrics.bearingX * layoutScale;
            const SR_MATH_NS::FVector2 size = glyph.metrics.size.CastToFloat() * layoutScale;
            const float_t y = currentLine.baselineY + glyph.metrics.bearingY * layoutScale - size.y;

            placements.push_back({ glyphIndex, { x, y }, size });
            bounds.IncludeQuad(x, y, size);

            if (glyph.codepoint.codepoint == 32u) { /// Space character.
                currentLine.spacePlacementIndices.push_back(placements.size() - 1);
            }

            penX = cursorX + glyph.metrics.advance * layoutScale;
            prevCode = glyph.codepoint;
        }
        finalizeLine();

        if (placements.empty()) {
            return false;
        }

        const float_t firstBaselineY = lines.front().baselineY;
        const float_t lastBaselineY = lines.back().baselineY;

        /// Горизонталь (TMP): Left — origin без сдвига; Center/Right — по ширине строки; GeometryCenter — по bounds блока.
        if (context.layoutSize.x > 0.f) {
            const bool justifyAllLines = context.horizontalAlignment == TextAlignmentHorizontal::Flush;
            const bool justifyExceptLast = context.horizontalAlignment == TextAlignmentHorizontal::Justified;

            if (justifyAllLines || justifyExceptLast) {
                for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
                    auto&& line = lines[lineIndex];
                    const bool isLastLine = lineIndex + 1 == lines.size();
                    if (justifyExceptLast && isLastLine) {
                        continue;
                    }

                    const float_t extra = context.layoutSize.x - line.penWidth;
                    if (extra <= 0.f || line.spacePlacementIndices.empty()) {
                        continue;
                    }

                    const float_t extraPerSpace =
                        extra / static_cast<float_t>(line.spacePlacementIndices.size());
                    ApplyLineJustification(placements, line, context.layoutSize.x, extraPerSpace);
                }
            }

            if (context.horizontalAlignment == TextAlignmentHorizontal::GeometryCenter) {
                TextMeshDetails::Bounds geometryBounds;
                for (auto&& placement : placements) {
                    geometryBounds.IncludeQuad(placement.pos.x, placement.pos.y, placement.size);
                }
                if (geometryBounds.IsValid()) {
                    const float_t blockCenterX = (geometryBounds.minX + geometryBounds.maxX) * 0.5f;
                    const float_t dx = context.layoutSize.x * 0.5f - blockCenterX;
                    for (auto&& placement : placements) {
                        placement.pos.x += dx;
                    }
                }
            }
            else {
                for (auto&& line : lines) {
                    float_t lineDx = 0.f;
                    switch (context.horizontalAlignment) {
                    case TextAlignmentHorizontal::Left:
                    case TextAlignmentHorizontal::Justified:
                    case TextAlignmentHorizontal::Flush:
                        break;
                    case TextAlignmentHorizontal::Center:
                        lineDx = (context.layoutSize.x - line.penWidth) * 0.5f;
                        break;
                    case TextAlignmentHorizontal::Right:
                        lineDx = context.layoutSize.x - line.penWidth;
                        break;
                    case TextAlignmentHorizontal::GeometryCenter:
                        break;
                    }

                    for (size_t i = line.placementBegin; i < line.placementEnd; ++i) {
                        placements[i].pos.x += lineDx;
                    }
                }
            }
        }

        bounds = {};
        for (auto&& placement : placements) {
            bounds.IncludeQuad(placement.pos.x, placement.pos.y, placement.size);
        }

        /// Вертикаль (TMP): Top/Capline/Midline/Baseline — по метрикам шрифта; Middle/Bottom — по bounds блока.
        if (context.layoutSize.y > 0.f) {
            float_t dy = 0.f;
            switch (context.verticalAlignment) {
            case TextAlignmentVertical::Top:
                dy = context.layoutSize.y - ascender - firstBaselineY;
                break;
            case TextAlignmentVertical::Capline:
                dy = context.layoutSize.y - capline - firstBaselineY;
                break;
            case TextAlignmentVertical::Midline:
                dy = context.layoutSize.y - midline - firstBaselineY;
                break;
            case TextAlignmentVertical::Baseline:
                dy = context.layoutSize.y - ascender - firstBaselineY;
                break;
            case TextAlignmentVertical::Bottom:
                dy = -descender - lastBaselineY;
                break;
            case TextAlignmentVertical::Middle:
                if (bounds.IsValid()) {
                    const float_t blockHeight = bounds.maxY - bounds.minY;
                    dy = (context.layoutSize.y - blockHeight) * 0.5f - bounds.minY;
                }
                break;
            }

            for (auto&& placement : placements) {
                placement.pos.y += dy;
            }
        }

        return true;
    }
}