//
// Created by Monika on 29.04.2026.
//

#ifndef SR_ENGINE_GRAPHICS_FONT_ASSET_H
#define SR_ENGINE_GRAPHICS_FONT_ASSET_H

#include <Graphics/Font/Font.h>
#include <Graphics/Font/Glyph.h>

#include <Utils/Resources/Asset.h>
#include <Utils/Resources/ResourceRef.h>

namespace SR_GRAPH_NS {
    class FontAtlas;
    class FontAtlasPage;
    class TextureData;

    class FontIndexer : public SR_UTILS_NS::Singleton<FontIndexer> {
        SR_REGISTER_SINGLETON(FontIndexer)
    public:
        bool IsSingletonCanBeDestroyed() const override { return false; }
        SR_NODISCARD uint16_t GetIndexForFont(SR_UTILS_NS::StringAtom id) const;
        SR_NODISCARD SR_UTILS_NS::StringAtom GetFontIdByIndex(uint16_t index) const;

    private:
        mutable SR_HTYPES_NS::FlatHashMap<SR_UTILS_NS::StringAtom, uint16_t> m_indexes;
        mutable std::vector<SR_UTILS_NS::StringAtom> m_fontIds;
        mutable uint16_t m_currentIndex = 0;

    };

    /// @extension(font)
    class FontAsset : public SR_UTILS_NS::Asset {
        SR_CLASS()
    public:
        bool BuildText(const std::string& text, float_t fontSize, std::vector<PositionedGlyph>& glyphs);

        /// Разрешение генерации atlas (pixels per em baseline); масштаб экранного размера: fontSize / GetSamplingPointSize().
        SR_NODISCARD float_t GetSamplingPointSize() const noexcept { return m_samplingPointSize; }

        SR_NODISCARD const SR_HTYPES_NS::SharedPtr<TextureData>& GetAtlasTexture(GlyphRenderType type, uint16_t page) const;
        SR_NODISCARD bool IsAtlasPageDirty(GlyphRenderType type, uint16_t page) const;
        SR_NODISCARD float GetKerning(GlyphKey left, GlyphKey right) const;
        SR_NODISCARD float_t GetFontAscender() const noexcept { return m_ascender; }
        SR_NODISCARD float_t GetFontDescender() const noexcept { return m_descender; }
        SR_NODISCARD float_t GetFontLineGap() const noexcept { return m_lineGap; }
        SR_NODISCARD float_t GetCapLineAscenderRatio() const noexcept;

        void OnAtlasPageUploaded(GlyphRenderType type, uint16_t page);
        void UpdateGlyphs();

    private:
        void OnAssetLoaded() override;
        void OnAssetUnloaded() override;

        void OnGlyphBitmapGenerated(const GlyphKey& key);

        static void ComputeGlyphBitmap(bool async, SR_GTYPES_NS::Font* pFont, FontDetails::Glyph* pGlyph, FontAsset* pAsset);

        FontDetails::Glyph* LoadGlyph(SR_GTYPES_NS::Font* pFont, const GlyphKey& key);
        SR_NODISCARD FontAtlasPage* GetAtlasPage(GlyphRenderType type, uint16_t page) const;

    private:
        /// @property
        /// @customArgs(pick: enabled, filter name: Font, relative: resources)
        /// @customArg(filter value: ttf) @group(Generation)
        SR_UTILS_NS::ResourceRef<SR_GTYPES_NS::Font> m_font;
        /// @property @group(Generation)
        std::vector<GlyphRange> m_preloadGlyphRanges;
        /// @property @group(Generation)
        float_t m_samplingPointSize = 86;
        /// @property @group(Generation)
        SR_MATH_NS::UVector2 m_atlasSize = { 1024, 1024 };
        /// @property @group(Generation)
        GlyphRenderType m_renderType = GlyphRenderType::MTSDF;
        /// @property @group(Generation)
        GlyphRenderType m_colorRenderType = GlyphRenderType::ColorBitmap;

        /// @property
        /// @customArgs(pick: enabled, filter name: Font, relative: resources)
        /// @customArg(filter value: font)
        std::vector<SR_UTILS_NS::ResourceRef<FontAsset>> m_fallbacks;

    private:
        float_t m_ascender = 0.f;
        float_t m_descender = 0.f;
        float_t m_lineGap = 0.f;

        std::recursive_mutex m_mutex;
        std::vector<GlyphKey> m_dirtyGlyphs;
        uint16_t m_fontId = 0;
        SR_HTYPES_NS::FlatHashMap<GlyphKey, FontDetails::Glyph> m_glyphs;
        SR_HTYPES_NS::SharedPtr<FontAtlas> m_atlases[SR_UTILS_NS::EnumTraits<GlyphRenderType>::NumItems];

    };
}

#endif //SR_ENGINE_GRAPHICS_FONT_ASSET_H
