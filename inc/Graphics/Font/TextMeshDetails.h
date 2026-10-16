//
// Created by Monika on 16.05.2026.
//

#ifndef SR_ENGINE_GRAPHICS_TEXT_MESH_DETAILS_H
#define SR_ENGINE_GRAPHICS_TEXT_MESH_DETAILS_H

#include <Graphics/Font/TextMesh.h>

namespace SR_GTYPES_NS::TextMeshDetails {
    struct GlyphPlacement {
        SR_UTILS_NS::SizeType glyphIndex = 0;
        SR_MATH_NS::FVector2 pos;
        SR_MATH_NS::FVector2 size;
    };

    struct TextLine {
        SR_UTILS_NS::SizeType placementBegin = 0;
        SR_UTILS_NS::SizeType placementEnd = 0;
        float_t penWidth = 0.f;
        float_t baselineY = 0.f;
        std::vector<SR_UTILS_NS::SizeType> spacePlacementIndices;
    };

    struct Bounds {
        float_t minX = std::numeric_limits<float_t>::infinity();
        float_t maxX = -std::numeric_limits<float_t>::infinity();
        float_t minY = std::numeric_limits<float_t>::infinity();
        float_t maxY = -std::numeric_limits<float_t>::infinity();

        void IncludeQuad(float_t x, float_t y, const SR_MATH_NS::FVector2& size) {
            minX = std::min(minX, x);
            maxX = std::max(maxX, x + size.x);
            minY = std::min(minY, y);
            maxY = std::max(maxY, y + size.y);
        }

        SR_NODISCARD bool IsValid() const noexcept {
            return std::isfinite(minX) && std::isfinite(maxX) && std::isfinite(minY) && std::isfinite(maxY);
        }
    };

    void ApplyLineJustification(
        SR_UTILS_NS::Vector<GlyphPlacement>& placements,
        TextLine& line,
        float_t layoutWidth,
        float_t extraPerSpace
    );

    struct GlyphPlacementContext {
        float_t fontSize = 0.f;
        bool kerning = false;
        TextAlignmentHorizontal horizontalAlignment = TextAlignmentHorizontal::Left;
        TextAlignmentVertical verticalAlignment = TextAlignmentVertical::Top;
        SR_MATH_NS::FVector2 layoutSize;
    };

    bool CalculateGlyphPlacements(
        FontAsset& fontAsset,
        const SR_UTILS_NS::Vector<PositionedGlyph>& glyphs,
        GlyphPlacementContext context,
        SR_UTILS_NS::Vector<GlyphPlacement>& placements
    );
}

#endif //SR_ENGINE_GRAPHICS_TEXT_MESH_DETAILS_H
