//
// Created by Monika on 30.04.2026.
//

#include <Graphics/Font/FontAtlas.h>

namespace SR_GRAPH_NS {
    FontAtlasPage::FontAtlasPage(SR_MATH_NS::USVector2 size, bool useRGBA)
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        , m_allocator(size.x, size.y)
        , m_pageSize(size)
        , m_useRGBA(useRGBA)
    {
        auto&& pBitmapData = (uint8_t*)SRMalloc(size.x * size.y * (useRGBA ? 4 : 1));
        TextureLoadInfo loadInfo;
        loadInfo.channels = useRGBA ? 4 : 1;
        loadInfo.mips = 1;
        m_textureData = TextureData::Create(size.x, size.y, pBitmapData, [](uint8_t* pData) {
            SRFree(pData);
        }, loadInfo);
    }

    void FontAtlasPage::CopyGlyphBitmap(const GlyphBitmap& bitmap, const SR_MATH_NS::USRect& destRect, SR_MATH_NS::USVector2 srcPixelSize) {
        SR_TRACY_ZONE;

        const uint32_t srcW = srcPixelSize.x;
        const uint32_t srcH = srcPixelSize.y;
        if (srcW == 0 || srcH == 0) {
            return;
        }

        /// При atlas padding destRect больше bitmap; кладём битмап по центру ячейки и заполняем gutter нейтральным фоном SDF.
        const uint32_t ox = (destRect.w >= srcW) ? (destRect.w - srcW) / 2u : 0u;
        const uint32_t oy = (destRect.h >= srcH) ? (destRect.h - srcH) / 2u : 0u;

        const bool useRGBAByBitmap = bitmap.type == GlyphRenderType::ColorBitmap || bitmap.type == GlyphRenderType::MSDF || bitmap.type == GlyphRenderType::MTSDF;
        const bool isRGBA = m_useRGBA;

        auto&& pData = m_textureData->GetDataMutable();

        for (uint32_t y = 0; y < destRect.h; ++y) {
            for (uint32_t x = 0; x < destRect.w; ++x) {
                const uint32_t destIndex = ((destRect.y + y) * m_pageSize.x + (destRect.x + x)) * (isRGBA ? 4 : 1);
                if (isRGBA) {
                    pData[destIndex + 0] = 255;
                    pData[destIndex + 1] = 255;
                    pData[destIndex + 2] = 255;
                    pData[destIndex + 3] = 255;
                }
                else {
                    pData[destIndex] = 255;
                }
            }
        }

        for (uint32_t y = 0; y < srcH; ++y) {
            for (uint32_t x = 0; x < srcW; ++x) {
                const uint32_t destIndex = ((destRect.y + oy + y) * m_pageSize.x + (destRect.x + ox + x)) * (isRGBA ? 4 : 1);
                const uint32_t srcIndex = (y * srcW + x) * (useRGBAByBitmap ? 4 : 1);

                if (useRGBAByBitmap && isRGBA) { /// RGBA bitmap into RGBA atlas
                    pData[destIndex + 0] = bitmap.data[srcIndex + 0];
                    pData[destIndex + 1] = bitmap.data[srcIndex + 1];
                    pData[destIndex + 2] = bitmap.data[srcIndex + 2];
                    pData[destIndex + 3] = bitmap.data[srcIndex + 3];
                }
                else if (!useRGBAByBitmap && isRGBA) { /// Grayscale bitmap into RGBA atlas
                    pData[destIndex + 0] = bitmap.data[srcIndex];
                    pData[destIndex + 1] = bitmap.data[srcIndex];
                    pData[destIndex + 2] = bitmap.data[srcIndex];
                    pData[destIndex + 3] = bitmap.data[srcIndex];
                }
                else if (useRGBAByBitmap) {
                    const uint8_t alpha = bitmap.data[srcIndex + 3];
                    const uint8_t r = bitmap.data[srcIndex + 0];
                    const uint8_t g = bitmap.data[srcIndex + 1];
                    const uint8_t b = bitmap.data[srcIndex + 2];
                    const auto gray = static_cast<uint8_t>(
                        (static_cast<float_t>(r * alpha) / 255.f) * 0.299f +
                        (static_cast<float_t>(g * alpha) / 255.f) * 0.587f +
                        (static_cast<float_t>(b * alpha) / 255.f) * 0.114f
                    );
                    pData[destIndex] = gray;
                }
                else {
                    pData[destIndex] = bitmap.data[srcIndex];
                }
            }
        }

        m_dirty = true;
    }

    void FontAtlasPage::OnTextureUploaded() {
        if (m_dirty) {
            m_dirty = false;
        }
        else {
            SRHalt("FontAtlasPage::OnTextureUploaded() : page is not dirty!");
        }
    }

    FontAtlas::FontAtlas(SR_MATH_NS::USVector2 size, bool useRGBA, uint8_t padding)
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        , m_pageSize(size)
        , m_useRGBA(useRGBA)
        , m_padding(padding)
    { }

    GlyphEntry* FontAtlas::GetOrCreate(const FontDetails::Glyph& glyph) {
        SR_TRACY_ZONE;

        if (glyph.bitmap.data.empty()) {
            return nullptr;
        }

        if (auto&& pIt = m_glyphs.find(glyph.codepoint); pIt != m_glyphs.end()) {
            return &pIt->second;
        }

        GlyphEntry entry;
        entry.codepoint = glyph.codepoint;
        entry.size = glyph.metrics.size;

        if (!PlaceGlyph(glyph, entry)) {
            return nullptr;
        }

        const float_t invW = 1.0f / static_cast<float_t>(m_pageSize.x);
        const float_t invH = 1.0f / static_cast<float_t>(m_pageSize.y);

        const uint32_t srcW = entry.size.x;
        const uint32_t srcH = entry.size.y;
        const uint32_t ox = (entry.rect.w >= srcW) ? (entry.rect.w - srcW) / 2u : 0u;
        const uint32_t oy = (entry.rect.h >= srcH) ? (entry.rect.h - srcH) / 2u : 0u;

        entry.atlas.uv0 = {
            static_cast<float_t>(entry.rect.x + ox) * invW,
            static_cast<float_t>(entry.rect.y + oy + srcH) * invH
        };
        entry.atlas.uv1 = {
            static_cast<float_t>(entry.rect.x + ox + srcW) * invW,
            static_cast<float_t>(entry.rect.y + oy) * invH
        };
        entry.atlas.page = m_pages.size() - 1;

        auto [resIt, _] = m_glyphs.emplace(entry.codepoint, entry);
        return &resIt->second;
    }

    bool FontAtlas::PlaceGlyph(const FontDetails::Glyph& glyph, GlyphEntry& out) {
        SR_TRACY_ZONE;

        if (m_pages.empty()) {
            m_pages.emplace_back(new FontAtlasPage(m_pageSize, m_useRGBA));
        }

        out.size = glyph.metrics.size;

        SR_MATH_NS::USVector2 paddedSize = glyph.metrics.size + SR_MATH_NS::USVector2(m_padding) * 2;
        if (!m_pages.back()->GetAllocator().Allocate(paddedSize.x, paddedSize.y, out.rect)) {
            if (m_pages.back()->GetAllocator().GetNodesCount() <= 1) {
                return false; /// Glyph too big
            }
            m_pages.emplace_back(new FontAtlasPage(m_pageSize, m_useRGBA));
            if (!m_pages.back()->GetAllocator().Allocate(paddedSize.x,paddedSize.y, out.rect)) {
                SRHalt("FontAtlas::PlaceGlyph() : failed to place glyph!");
                return false; /// Something went wrong...
            }
        }

        m_pages.back()->CopyGlyphBitmap(glyph.bitmap, out.rect, glyph.metrics.size);

        /// UV должны охватывать весь битмап (включая SDF-halo для MSDF/MTSDF). Inset здесь только обрезал поля atlas.
        return true;
    }

    const FontAtlasPage::Ptr& FontAtlas::GetPage(uint16_t page) const {
        if (page >= m_pages.size()) {
            static FontAtlasPage::Ptr emptyPage;
            return emptyPage;
        }
        return m_pages[page];
    }
}