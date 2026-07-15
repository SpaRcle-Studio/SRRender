//
// Created by Monika on 24.05.2023.
//

#ifndef SR_ENGINE_GLYPH_H
#define SR_ENGINE_GLYPH_H

#include <Graphics/Font/FreeType.h>
#include <Graphics/Font/GlyphRenderType.h>

#include <Utils/Common/NonCopyable.h>
#include <Utils/Math/Vector2.h>
#include <Utils/Math/Rect.h>
#include <Utils/Serialization/Serializable.h>

namespace SR_UTILS_NS {
    class VertexDataBuffer;
}

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(GlyphRangeType, uint8_t,
        ASCII, Latin1, Cyrilic, Custom
    )

    struct GlyphKey {
        uint32_t codepoint = 0;

        SR_NODISCARD bool operator==(const GlyphKey& other) const noexcept {
            return codepoint == other.codepoint;
        }

        SR_NODISCARD static bool DecodeUTF8(const std::string& str, uint64_t& i, uint32_t& out);
        SR_NODISCARD static bool NextGlyphKey(const std::string& text, uint64_t& i, GlyphKey& key);
    };

    struct GlyphRange : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        using Super = SR_UTILS_NS::Serializable;

        GlyphRange() = default;
        GlyphRange(GlyphRangeType type, uint32_t start, uint32_t end)
            : Super()
            , type(type)
            , start(start)
            , end(end)
        { }

        /// @property
        GlyphRangeType type = GlyphRangeType::Custom;
        /// @property @condition(This.type == GlyphRangeType::Custom)
        uint32_t start = 0;
        /// @property @condition(This.type == GlyphRangeType::Custom)
        uint32_t end = 0;
    };

    struct GlyphBitmap {
        GlyphRenderType type = GlyphRenderType::SDF;
        SR_HTYPES_NS::FastMemoryArray<uint8_t, true, uint32_t> data;
        bool generated = false;
    };

    struct GlyphAtlas {
        SR_MATH_NS::FVector2 uv0;
        SR_MATH_NS::FVector2 uv1;
        uint16_t page = SR_ID_INVALID;
    };

    namespace FontDetails {
        struct GlyphMetrics {
            float_t advance  = 0.f; // сдвиг курсора
            float_t bearingX = 0.f; // от cursor до левого края
            float_t bearingY = 0.f; // от baseline до верхнего края

            SR_MATH_NS::USVector2 size;

            float_t sdfRange = 0.f;
            uint16_t fontId = 0;
        };

        struct Glyph {
            GlyphKey codepoint;
            GlyphMetrics metrics;
            GlyphBitmap bitmap;
        };

        SR_INLINE_STATIC GlyphRange ASCIIRange = { GlyphRangeType::ASCII, 32, 126 };
        SR_INLINE_STATIC GlyphRange Latin1Range = { GlyphRangeType::Latin1, 160, 255 };
        SR_INLINE_STATIC GlyphRange CyrillicRange = { GlyphRangeType::Cyrilic, 0x0400, 0x04FF };
        SR_INLINE_STATIC GlyphRange* GlyphRanges[3] = { &ASCIIRange, &Latin1Range, &CyrillicRange };
    }

    struct PositionedGlyph {
        GlyphKey codepoint;
        FontDetails::GlyphMetrics metrics;
        GlyphAtlas atlas;

        SR_MATH_NS::FVector2 pos0;
        SR_MATH_NS::FVector2 pos1;

        uint32_t glyphIndex = 0;
        uint32_t lineIndex  = 0;

        /// layoutScale экранная доля метрик atlas: fontSize / FontAsset::GetSamplingPointSize()
        void AddInstance(uint32_t index, const SR_MATH_NS::FVector2& pos, const SR_MATH_NS::FVector2& size, SR_UTILS_NS::VertexDataBuffer& buffer) const;
    };

    //// ============================================== OLD GLYPH CODE =================================================

    struct GlyphMetrics {
        /// Позиция по горизонтали
        int32_t posX = 0;
        /// Позиция по вертикали (от базовой линии)
        int32_t posY = 0;
        /// Ширина глифа
        int32_t width = 0;
        /// Высота глифа
        int32_t height = 0;

        int32_t left = 0;
        int32_t top = 0;

        int32_t advanceX = 0;
        int32_t advanceY = 0;
    };

    class Glyph : public SR_UTILS_NS::NonCopyable {
    public:
        using Super = SR_UTILS_NS::NonCopyable;
        using Ptr = std::shared_ptr<Glyph>;

    public:
#ifdef SR_USE_FREETYPE
        Glyph(FT_Glyph pGlyph, FT_Render_Mode renderMode);
#endif
        ~Glyph() override;

    public:
        SR_NODISCARD int32_t GetPosX() const noexcept;
        SR_NODISCARD int32_t GetPosY() const noexcept;
        SR_NODISCARD uint32_t GetSize() const noexcept;
        SR_NODISCARD uint32_t GetWidth() const noexcept;
        SR_NODISCARD uint32_t GetHeight() const noexcept;
        SR_NODISCARD uint32_t GetPixelSize() const noexcept;
        SR_NODISCARD GlyphMetrics& GetMetrics() noexcept;

    #ifdef SR_USE_FREETYPE
        SR_NODISCARD FT_Glyph GetGlyph() const noexcept;
    #endif

    private:
    #ifdef SR_USE_FREETYPE
        FT_Render_Mode m_renderMode;
        FT_Glyph m_glyph = nullptr;
    #endif
        GlyphMetrics m_metrics = { };

    };

    class GlyphImage : public SR_UTILS_NS::NonCopyable {
    public:
        using Ptr = std::shared_ptr<GlyphImage>;

    public:
        GlyphImage() = default;
        ~GlyphImage() override;

    public:
        SR_NODISCARD static GlyphImage::Ptr Create(const Glyph::Ptr& pGlyph, bool needInit);
        SR_NODISCARD uint8_t* GetData() const { return m_data; }

        void InsertTo(uint8_t* pTarget, int32_t top, uint32_t sizeX, uint32_t sizeY, bool invertX, bool invertY);
        void Debug(uint8_t* pTarget, int32_t top, uint32_t sizeX, uint32_t sizeY, bool invertX, bool invertY);

    private:
        SR_NODISCARD bool Init();

    private:
        uint8_t* m_data = nullptr;
        Glyph::Ptr m_glyph;

    };
}

template<> struct std::hash<SR_GRAPH_NS::GlyphKey> {
    SR_NODISCARD constexpr size_t operator()(const SR_GRAPH_NS::GlyphKey& v) const noexcept {
        return v.codepoint;
    }
};

#endif //SR_ENGINE_GLYPH_H
