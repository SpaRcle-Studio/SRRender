//
// Created by Monika on 30.10.2022.
//

#ifndef SR_ENGINE_TEXTBUILDER_H
#define SR_ENGINE_TEXTBUILDER_H

#include <Utils/Common/NonCopyable.h>
#include <Graphics/Font/FreeType.h>
#include <Graphics/Font/Glyph.h>

namespace SR_GTYPES_NS {
    class Font;
}

namespace SR_GRAPH_NS {
    class TextBuilder : SR_UTILS_NS::NonCopyable {
        using Super = SR_UTILS_NS::NonCopyable;
        using FontPtr = SR_GTYPES_NS::Font*;
        using StringType = std::u32string;
    public:
        explicit TextBuilder(FontPtr pFont);
        ~TextBuilder() override;

    public:
        SR_NODISCARD uint32_t GetSize() const noexcept;
        SR_NODISCARD uint32_t GetWidth() const noexcept { return m_imageWidth; }
        SR_NODISCARD uint32_t GetHeight() const noexcept { return m_imageHeight; }
        SR_NODISCARD uint8_t* GetData() const noexcept { return m_textureData; }
        SR_NODISCARD ImageFormat GetColorFormat() const noexcept;
        SR_NODISCARD uint32_t GetFontSize() const noexcept { return m_fontSize; }

        SR_NODISCARD int32_t CalculateTextWidth(const char* text);

        bool Build(const char* text);
        bool Build(StringType text);

        void SetFontSize(uint32_t size) { m_fontSize = size; }
        void SetKerning(bool enabled);
        void SetDebug(bool enabled);

    private:
        void InitFontSize();
        void Clear();

        bool ParseGlyphs(const StringType& text);

        uint32_t PreProcess(const StringType& text, uint32_t iterator);
        void PreProcessImpl(const StringType& text, uint32_t begin, uint32_t end);

    private:
        FT_Render_Mode m_renderMode = FT_RENDER_MODE_NORMAL;

        FontPtr m_font = nullptr;

        std::vector<Glyph::Ptr> m_glyphs;

        bool m_kerning = false;
        bool m_debug = false;

        uint32_t m_fontSize = 12;

        uint32_t m_align = 0;
        uint32_t m_valign = 110;
        uint32_t m_space = 24;

        int32_t m_top = 0;

        uint32_t m_imageHeight = 0;
        uint32_t m_imageWidth = 0;
        uint8_t* m_textureData = nullptr;

        bool m_needParse = false;

    };
}

#endif //SR_ENGINE_TEXTBUILDER_H
