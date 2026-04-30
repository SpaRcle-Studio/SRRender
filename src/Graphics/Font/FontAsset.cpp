//
// Created by Monika on 29.04.2026.
//

#include <Graphics/Font/FontAsset.h>
#include <Graphics/Font/FontAtlas.h>

#include <Codegen/FontAsset.generated.hpp>

namespace SR_GRAPH_NS {
    SR_NODISCARD uint16_t FontIndexer::GetIndexForFont(SR_UTILS_NS::StringAtom id) const {
        SR_TRACY_ZONE;
        if (m_currentIndex == std::numeric_limits<decltype(m_currentIndex)>::max()) {
            SRHalt("FontIndexer::GetIndexForFont() : overflow!");
            return 0;
        }
        if (auto&& pIt = m_indexes.find(id); pIt != m_indexes.end()) {
            return pIt->second;
        }
        const uint16_t newIndex = m_currentIndex++;
        m_indexes[id] = newIndex;
        m_fontIds.resize(m_currentIndex);
        m_fontIds[newIndex] = id;
        return newIndex;
    }

    SR_UTILS_NS::StringAtom FontIndexer::GetFontIdByIndex(uint16_t index) const {
        SR_TRACY_ZONE;
        if (index >= m_fontIds.size()) {
            SRHalt("FontIndexer::GetFontIdByIndex() : index out of range!");
            return SR_UTILS_NS::StringAtom();
        }
        return m_fontIds[index];
    }

    bool FontAsset::BuildText(const std::string& text, float_t fontSize, std::vector<PositionedGlyph>& glyphs) {
        SR_TRACY_ZONE;

        glyphs.clear();

        auto&& pFont = m_font.GetResource();
        if (!pFont) {
            SR_ERROR("FontAsset::BuildText() : failed to load \"{}\" font!", m_font.GetId());
            return false;
        }

        GlyphKey key;
        uint32_t glyphIndex = 0;
        for (uint64_t i = 0; i < text.size(); ) {
            if (!GlyphKey::NextGlyphKey(text, i, key)) {
                ++i;
                continue;
            }

            auto&& pGlyph = LoadGlyph(pFont.Get(), key);
            if (!pGlyph) {
                continue;
            }

            uint8_t padding = 0;
            if (pGlyph->bitmap.type == GlyphRenderType::SDF || pGlyph->bitmap.type == GlyphRenderType::MSDF || pGlyph->bitmap.type == GlyphRenderType::MTSDF) {
                padding = static_cast<uint8_t>(ceil(pGlyph->metrics.sdfRange) + 1);
            }

            auto&& pAtlas = m_atlases[SR_UTILS_NS::EnumReflector::AsInt(pGlyph->bitmap.type)];
            if (!pAtlas) {
                const bool useRGBA = pGlyph->bitmap.type == GlyphRenderType::ColorBitmap || pGlyph->bitmap.type == GlyphRenderType::MSDF || pGlyph->bitmap.type == GlyphRenderType::MTSDF;
                pAtlas = new FontAtlas(m_atlasSize.CastToUSInt(), useRGBA, padding);
            }

            if (!pGlyph->bitmap.generated) {
                pGlyph->bitmap.generated = true;
                if (pGlyph->bitmap.type == GlyphRenderType::MSDF || pGlyph->bitmap.type == GlyphRenderType::MTSDF) {
                    const bool isMTSDF = pGlyph->bitmap.type == GlyphRenderType::MTSDF;
                    const uint32_t paddedWidth = pGlyph->metrics.size.x + padding * 2;
                    const uint32_t paddedHeight = pGlyph->metrics.size.y + padding * 2;
                    if (!pFont->GenerateMSDFOrMTSDF(pGlyph->codepoint.codepoint, pGlyph->bitmap.data, paddedWidth, paddedHeight, pGlyph->metrics.sdfRange, padding, isMTSDF)) {
                        SR_ERROR("FontAsset::BuildText() : failed to generate MSDF (or MTSDF) for codepoint {}!", pGlyph->codepoint.codepoint);
                        continue;
                    }
                }
            }

            auto&& positionedGlyph = glyphs.emplace_back();
            positionedGlyph.metrics = pGlyph->metrics;
            positionedGlyph.codepoint = key;
            positionedGlyph.glyphIndex = glyphIndex++;

            if (auto&& pGlyphEntry = pAtlas->GetOrCreate(*pGlyph)) {
                positionedGlyph.atlas = pGlyphEntry->atlas;
            }
        }

        return true;
    }

    FontDetails::Glyph* FontAsset::LoadGlyph(SR_GTYPES_NS::Font* pFont, const GlyphKey& key) {
        SR_TRACY_ZONE;

        static SR_THREAD_LOCAL SR_HTYPES_NS::FastMemoryArray<float> sdf;

        if (auto&& pIt = m_glyphs.find(key); pIt != m_glyphs.end()) {
            return &pIt->second;
        }

#ifdef SR_USE_FREETYPE
        auto&& pFace = pFont->GetFontFace();
        if (!pFace) {
            SR_ERROR("FontAsset::OnAssetLoaded() : \"{}\" font have invalid face!", m_font.GetId());
            return nullptr;
        }
        FT_UInt glyphIndex = FT_Get_Char_Index(pFace, key.codepoint);
        if (glyphIndex == 0) {
            return nullptr;
        }

        const bool hasColor = pFont->HasColor();

        GlyphRenderType renderType = hasColor ? GlyphRenderType::ColorBitmap : GlyphRenderType::MTSDF;

        FT_Int32 loadFlags = FT_LOAD_DEFAULT;
        switch (renderType) {
            case GlyphRenderType::Bitmap: loadFlags |= FT_LOAD_RENDER; break;
            case GlyphRenderType::ColorBitmap: loadFlags |= FT_LOAD_COLOR; break;
            case GlyphRenderType::SDF: loadFlags |= FT_LOAD_NO_BITMAP; break;
            case GlyphRenderType::MTSDF:
            case GlyphRenderType::MSDF: loadFlags |= FT_LOAD_NO_BITMAP; break;
            default:
                SRHaltOnce0();
                break;
        }

        if (FT_Load_Glyph(pFace, glyphIndex, loadFlags)) {
            return nullptr;
        }

        const bool noBitmap = pFace->glyph->format != FT_GLYPH_FORMAT_BITMAP || pFace->glyph->bitmap.width == 0;
        const bool isSDF = renderType == GlyphRenderType::SDF || renderType == GlyphRenderType::MSDF || renderType == GlyphRenderType::MTSDF;
        if (noBitmap && FT_Render_Glyph(pFace->glyph, isSDF ? FT_RENDER_MODE_MONO : FT_RENDER_MODE_NORMAL)) {
            return nullptr;
        }

        auto& glyph = m_glyphs[key];
        glyph.codepoint = key;
        glyph.bitmap.type = renderType;
        glyph.metrics.fontId = m_fontId;
        glyph.metrics.advance = static_cast<float_t>(pFace->glyph->advance.x >> 6);
        glyph.metrics.bearingX = static_cast<float_t>(pFace->glyph->bitmap_left);
        glyph.metrics.bearingY = static_cast<float_t>(pFace->glyph->bitmap_top);
        glyph.metrics.size.x = static_cast<uint16_t>(pFace->glyph->bitmap.width);
        glyph.metrics.size.y = static_cast<uint16_t>(pFace->glyph->bitmap.rows);
        glyph.metrics.sdfRange = SR_CLAMP(m_samplingPointSize * 0.1f, 4.0f, 16.0f);

        if (renderType == GlyphRenderType::ColorBitmap) {
            const FT_Bitmap& bmp = pFace->glyph->bitmap;
            glyph.bitmap.data.resize(bmp.width * bmp.rows * 4);

            for (int y = 0; y < bmp.rows; ++y) {
                const uint8_t* row = bmp.pitch > 0 ? bmp.buffer + y * bmp.pitch : bmp.buffer + (bmp.rows - 1 - y) * (-bmp.pitch);
                for (int x = 0; x < bmp.width; ++x) {
                    const uint8_t* src = row + x * 4;
                    uint8_t* dst = &glyph.bitmap.data[(y * bmp.width + x) * 4];
                    if (bmp.pixel_mode == FT_PIXEL_MODE_BGRA) {
                        // BGRA → RGBA
                        dst[0] = src[2];
                        dst[1] = src[1];
                        dst[2] = src[0];
                        dst[3] = src[3];
                    }
                    else if (bmp.pixel_mode == FT_PIXEL_MODE_GRAY) {
                        // Grayscale → RGBA
                        dst[0] = src[0];
                        dst[1] = src[0];
                        dst[2] = src[0];
                        dst[3] = 255;
                    }
                }
            }
        }
        else if (renderType == GlyphRenderType::Bitmap) {
             const FT_Bitmap& bmp = pFace->glyph->bitmap;
             glyph.bitmap.data.resize(bmp.width * bmp.rows);
             for (int y = 0; y < bmp.rows; ++y) {
                 for (int x = 0; x < bmp.width; ++x) {
                     const uint8_t* srcPixel = &bmp.buffer[y * bmp.pitch + x];
                     uint8_t* dstPixel = &glyph.bitmap.data[y * bmp.width + x];
                     *dstPixel = *srcPixel;
                 }
             }
        }
        else if (renderType == GlyphRenderType::SDF) {
            FT_Bitmap& bmp = pFace->glyph->bitmap;
            FreeTypeGenerateSDF(bmp, sdf, bmp.width, bmp.rows, glyph.metrics.sdfRange);
            glyph.bitmap.data.resize(sdf.size() * sizeof(float_t));
            memcpy(glyph.bitmap.data.data(), sdf.data(), glyph.bitmap.data.size());
        }

        return &glyph;
    #else
        SR_WARN("FontAsset::OnAssetLoaded() : freetype is not supported!");
        return nullptr;
    #endif
    }

    void FontAsset::OnAssetUnloaded() {
        SR_TRACY_ZONE;

        m_glyphs.clear();
        for (auto&& pAtlas : m_atlases) {
            pAtlas.AutoFree();
        }
    }

    void FontAsset::OnAssetLoaded() {
        SR_TRACY_ZONE;

        if (m_font.GetId().empty()) {
            return;
        }

        m_fontId = FontIndexer::Instance().GetIndexForFont(GetResourceId());

        auto&& pFont = m_font.GetResource();
        if (!pFont) {
            SR_ERROR("FontAsset::OnAssetLoaded() : failed to load \"{}\" font!", m_font.GetId());
            return;
        }

        m_glyphs.reserve(1024);

        for (GlyphRange& range : m_preloadGlyphRanges) {
            if (range.type != GlyphRangeType::Custom) {
                range = *FontDetails::GlyphRanges[SR_UTILS_NS::EnumReflector::AsInt(range.type)];
            }
            for (uint32_t cp = range.start; cp <= range.end; ++cp) {
                LoadGlyph(pFont.Get(), GlyphKey { cp });
            }
        }

        SR_LOG("FontAsset::OnAssetLoaded() : loaded {} glyphs for \"{}\"", m_glyphs.size(), GetResourceId());
    }

    const SR_HTYPES_NS::SharedPtr<TextureData>& FontAsset::GetAtlasTexture(GlyphRenderType type, uint16_t page) const {
        SR_TRACY_ZONE;
        if (auto&& pPage = GetAtlasPage(type, page)) {
            return pPage->GetTextureData();
        }
        static const SR_HTYPES_NS::SharedPtr<TextureData> emptyTextureData;
        return emptyTextureData;
    }

    FontAtlasPage* FontAsset::GetAtlasPage(GlyphRenderType type, uint16_t page) const {
        SR_TRACY_ZONE;
        auto&& pAtlas = m_atlases[SR_UTILS_NS::EnumReflector::AsInt(type)];
        if (!pAtlas) {
            return nullptr;
        }

        auto&& pPage = pAtlas->GetPage(page);
        return const_cast<FontAtlasPage*>(pPage.Get());
    }

    bool FontAsset::IsAtlasPageDirty(GlyphRenderType type, uint16_t page) const {
        SR_TRACY_ZONE;
        auto&& pPage = GetAtlasPage(type, page);
        return pPage ? pPage->IsDirty() : false;
    }

    void FontAsset::OnAtlasPageUploaded(GlyphRenderType type, uint16_t page) {
        SR_TRACY_ZONE;
        if (auto&& pPage = GetAtlasPage(type, page)) {
            pPage->OnTextureUploaded();
        }
    }
}