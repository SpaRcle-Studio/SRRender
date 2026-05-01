//
// Created by Monika on 30.04.2026.
//

#ifndef SR_ENGINE_GRAPHICS_FONT_ATLAS_H
#define SR_ENGINE_GRAPHICS_FONT_ATLAS_H

#include <Graphics/Font/Glyph.h>
#include <Graphics/Loaders/TextureLoader.h>

#include <Utils/Types/SharedPtr.h>
#include <Utils/Memory/SkylineAllocator.h>

namespace SR_GRAPH_NS {
    struct GlyphEntry {
        GlyphKey codepoint;
        GlyphAtlas atlas;

        SR_MATH_NS::USRect rect; /// Пиксельные координаты в атласе
        SR_MATH_NS::USVector2 size; /// Размер в пикселях (с padding)

        uint32_t lastUsedFrame = 0;
        uint32_t refCount = 0;
    };

    class FontAtlasPage : public SR_HTYPES_NS::SharedPtr<FontAtlasPage> {
        using Super = SR_HTYPES_NS::SharedPtr<FontAtlasPage>;
    public:
        FontAtlasPage(SR_MATH_NS::USVector2 size, bool useRGBA);

        SR_NODISCARD SR_UTILS_NS::SkylineAllocator& GetAllocator() { return m_allocator; }
        SR_NODISCARD const TextureData::Ptr& GetTextureData() const { return m_textureData; }
        SR_NODISCARD bool IsDirty() const { return m_dirty; }

        /// srcPixelSize — реальный размер bitmap.data (glyph.metrics.size); destRect может быть шире из‑за atlas padding.
        void CopyGlyphBitmap(const GlyphBitmap& bitmap, const SR_MATH_NS::USRect& destRect, SR_MATH_NS::USVector2 srcPixelSize);
        void OnTextureUploaded();

    private:
        SR_MATH_NS::USVector2 m_pageSize;
        SR_UTILS_NS::SkylineAllocator m_allocator;
        TextureData::Ptr m_textureData;
        bool m_useRGBA = false;
        bool m_dirty = false;

    };

    class FontAtlas : public SR_HTYPES_NS::SharedPtr<FontAtlas> {
        using Super = SR_HTYPES_NS::SharedPtr<FontAtlas>;
    public:
        FontAtlas(SR_MATH_NS::USVector2 size, bool useRGBA, uint8_t padding);

        void UpdateGlyphBitmap(const FontDetails::Glyph& glyph);

        SR_NODISCARD GlyphEntry* GetOrCreate(const FontDetails::Glyph& glyph, bool allowEmpty);
        SR_NODISCARD const FontAtlasPage::Ptr& GetPage(uint16_t page) const;

    private:
        SR_NODISCARD bool PlaceGlyph(const FontDetails::Glyph& glyph, GlyphEntry& out);

    private:
        std::vector<FontAtlasPage::Ptr> m_pages;
        SR_HTYPES_NS::FlatHashMap<GlyphKey, GlyphEntry> m_glyphs;
        bool m_useRGBA = false;
        SR_MATH_NS::USVector2 m_pageSize;
        uint16_t m_padding = 0;

    };
}

#endif //SR_ENGINE_GRAPHICS_FONT_ATLAS_H
