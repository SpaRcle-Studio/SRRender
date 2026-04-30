//
// Created by Monika on 24.05.2023.
//

#include <Graphics/Font/Glyph.h>

#include <Codegen/Glyph.generated.hpp>

namespace SR_GRAPH_NS {
    bool GlyphKey::DecodeUTF8(const std::string& str, size_t& i, uint32_t& out) {
        const uint8_t c = static_cast<uint8_t>(str[i]);

        if (c < 0x80) {
            out = c;
            ++i;
            return true;
        }
        else if ((c >> 5) == 0x6) { // 110xxxxx
            if (i + 1 >= str.size()) return false;
            out = ((c & 0x1F) << 6) | (static_cast<uint8_t>(str[i+1]) & 0x3F);
            i += 2;
            return true;
        }
        else if ((c >> 4) == 0xE) { // 1110xxxx
            if (i + 2 >= str.size()) return false;
            out = ((c & 0x0F) << 12) |
                  ((static_cast<uint8_t>(str[i+1]) & 0x3F) << 6) |
                  (static_cast<uint8_t>(str[i+2]) & 0x3F);
            i += 3;
            return true;
        }
        else if ((c >> 3) == 0x1E) { // 11110xxx
            if (i + 3 >= str.size()) return false;
            out = ((c & 0x07) << 18) |
                  ((static_cast<uint8_t>(str[i+1]) & 0x3F) << 12) |
                  ((static_cast<uint8_t>(str[i+2]) & 0x3F) << 6) |
                  (static_cast<uint8_t>(str[i+3]) & 0x3F);
            i += 4;
            return true;
        }

        return false;
    }

    bool GlyphKey::NextGlyphKey(const std::string& text, size_t& i, GlyphKey& key) {
        uint32_t cp;
        if (!DecodeUTF8(text, i, cp)) {
            return false;
        }
        key = GlyphKey{ cp };
        return true;
    }

#ifdef SR_USE_FREETYPE
    Glyph::Glyph(FT_Glyph pGlyph, FT_Render_Mode renderMode)
        : Super()
        , m_renderMode(renderMode)
        , m_glyph(pGlyph)
    {
        FT_Glyph_To_Bitmap(&m_glyph, m_renderMode, 0, 1);
        FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)m_glyph;
        FT_Bitmap bitmap = bitmap_glyph->bitmap;

        m_metrics.advanceX = m_glyph->advance.x;
        m_metrics.advanceY = m_glyph->advance.y;
        m_metrics.left = bitmap_glyph->left;
        m_metrics.top = bitmap_glyph->top;
        m_metrics.width = bitmap.width;
        m_metrics.height = bitmap.rows;
    }
#endif

    Glyph::~Glyph() {
    #ifdef SR_USE_FREETYPE
        if (m_glyph) {
            FT_Done_Glyph(m_glyph);
            m_glyph = nullptr;
        }
    #endif
    }

    uint32_t Glyph::GetSize() const noexcept {
        return GetWidth() * GetHeight() * GetPixelSize();
    }

    uint32_t Glyph::GetPixelSize() const noexcept {
        return 4;
    }

    GlyphMetrics& Glyph::GetMetrics() noexcept {
        return m_metrics;
    }

    uint32_t Glyph::GetWidth() const noexcept {
        return m_metrics.width;
    }

    uint32_t Glyph::GetHeight() const noexcept {
        return m_metrics.height;
    }

#ifdef SR_USE_FREETYPE
    FT_Glyph Glyph::GetGlyph() const noexcept {
        return m_glyph;
    }
#endif

    int32_t Glyph::GetPosX() const noexcept {
        return m_metrics.posX;
    }

    int32_t Glyph::GetPosY() const noexcept {
        return m_metrics.posY;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    GlyphImage::Ptr GlyphImage::Create(const Glyph::Ptr& pGlyph, bool needInit) {
        auto&& pGlyphImage = std::make_shared<GlyphImage>();

        pGlyphImage->m_glyph = pGlyph;

        if (needInit && !pGlyphImage->Init()) {
            return nullptr;
        }

        return pGlyphImage;
    }


    GlyphImage::~GlyphImage() {
        SR_SAFE_DELETE_ARRAY_PTR(m_data);
    }

    bool GlyphImage::Init() {
        if (m_glyph->GetSize() == 0) {
            return false;
        }

        m_data = new uint8_t[m_glyph->GetSize()];

    #ifdef SR_USE_FREETYPE
        FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)m_glyph->GetGlyph();
        FT_Bitmap bitmap = bitmap_glyph->bitmap;

        uint8_t* pBuffer = bitmap.buffer;

        const uint32_t pixelSize = m_glyph->GetPixelSize();

        for (uint32_t x = 0; x < m_glyph->GetWidth(); ++x) {
            for (uint32_t y = 0; y < m_glyph->GetHeight(); ++y) {
                const uint32_t dst = x * pixelSize + y * m_glyph->GetWidth() * pixelSize;

                if (pixelSize == 1) {
                    const uint32_t src = x + y * bitmap.pitch;
                    m_data[dst] = 255 - pBuffer[src];
                }
                else if (bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
                    m_data[dst + 0] = pBuffer[dst + 2];
                    m_data[dst + 1] = pBuffer[dst + 1];
                    m_data[dst + 2] = pBuffer[dst + 0];
                    m_data[dst + 3] = pBuffer[dst + 3];
                }
                else {
                    const uint32_t src = x + y * bitmap.pitch;
                    m_data[dst + 0] = uint8_t(255 - pBuffer[src]);
                    m_data[dst + 1] = uint8_t(255 - pBuffer[src]);
                    m_data[dst + 2] = uint8_t(255 - pBuffer[src]);
                    m_data[dst + 3] = uint8_t(      pBuffer[src]);
                }
            }
        }
    #endif

        return true;
    }

    void GlyphImage::InsertTo(uint8_t* pTarget, int32_t top, uint32_t sizeX, uint32_t sizeY, bool invertX, bool invertY) {
    #ifdef SR_USE_FREETYPE
        const int32_t posX = m_glyph->GetPosX();
        const int32_t posY = m_glyph->GetPosY();

        const uint32_t pixelSize = m_glyph->GetPixelSize();
        const uint32_t width = m_glyph->GetWidth();
        const uint32_t height = m_glyph->GetHeight();

        FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)m_glyph->GetGlyph();
        FT_Bitmap bitmap = bitmap_glyph->bitmap;
        uint8_t* pBuffer = bitmap.buffer;

        for (uint32_t x = 0; x < width; ++x) {
            for (uint32_t y = 0; y < height; ++y) {
                uint32_t src = 0;

                if (bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
                    src = x * pixelSize + y * width * pixelSize;
                    if (pBuffer[src + (pixelSize - 1)] == 0) {
                        continue;
                    }
                }
                else {
                    src = x + y * bitmap.pitch;
                    if (pBuffer[src] == 0) {
                        continue;
                    }
                }

                const int32_t rawDstX = posX + static_cast<int32_t>(x);
                const int32_t rawDstY = posY + static_cast<int32_t>(y) - top;
                const int32_t dstX = invertX ? (static_cast<int32_t>(sizeX) - 1 - rawDstX) : rawDstX;
                const int32_t dstY = invertY ? (static_cast<int32_t>(sizeY) - 1 - rawDstY) : rawDstY;

                if (dstX < 0 || dstX >= static_cast<int32_t>(sizeX) || dstY < 0 || dstY >= static_cast<int32_t>(sizeY)) {
                    continue;
                }

                const int32_t dst = dstX * static_cast<int32_t>(pixelSize) + dstY * static_cast<int32_t>(sizeX) * static_cast<int32_t>(pixelSize);

                if (bitmap.pixel_mode == FT_PIXEL_MODE_BGRA) {
                    *(pTarget + dst + 0) = *(pBuffer + src + 2);
                    *(pTarget + dst + 1) = *(pBuffer + src + 1);
                    *(pTarget + dst + 2) = *(pBuffer + src + 0);
                    *(pTarget + dst + 3) = *(pBuffer + src + 3);
                }
                else {
                    *(pTarget + dst + 0) = 0;
                    *(pTarget + dst + 1) = 0;
                    *(pTarget + dst + 2) = 0;

                    const float a = *(pBuffer + src) / 255.0f;

                    *(pTarget + dst + 3) = uint8_t(a * 255 + (1 - a) * *(pTarget + dst + 3));
                }
            }
        }
    #endif
    }

    void GlyphImage::Debug(uint8_t* pTarget, int32_t top, uint32_t sizeX, uint32_t sizeY, bool invertX, bool invertY) {
        const int32_t posX = m_glyph->GetPosX();
        const int32_t posY = m_glyph->GetPosY();

        const uint32_t pixelSize = m_glyph->GetPixelSize();
        const uint32_t width = m_glyph->GetWidth();
        const uint32_t height = m_glyph->GetHeight();

        for (uint32_t x = 0; x < width; ++x) {
            for (uint32_t y = 0; y < height; ++y) {
                if (x != 0 && y != 0 && x + 1 != width && y + 1 != height) {
                    continue;
                }

                const int32_t rawDstX = posX + static_cast<int32_t>(x);
                const int32_t rawDstY = posY + static_cast<int32_t>(y) - top;
                const int32_t dstX = invertX ? (static_cast<int32_t>(sizeX) - 1 - rawDstX) : rawDstX;
                const int32_t dstY = invertY ? (static_cast<int32_t>(sizeY) - 1 - rawDstY) : rawDstY;

                if (dstX < 0 || dstX >= static_cast<int32_t>(sizeX) || dstY < 0 || dstY >= static_cast<int32_t>(sizeY)) {
                    continue;
                }

                const uint32_t dst = dstX * pixelSize + dstY * sizeX * pixelSize;

                *(pTarget + dst + 0) = 255;
                *(pTarget + dst + 1) = 0;
                *(pTarget + dst + 2) = 0;
                *(pTarget + dst + 3) = 255;
            }
        }
    }
}