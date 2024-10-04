//
// Created by Monika on 14.02.2022.
//

#ifndef SR_ENGINE_FONT_H
#define SR_ENGINE_FONT_H

#include <Utils/Resources/IResource.h>
#include <Graphics/Font/FreeType.h>

namespace SR_GTYPES_NS {
    class SR_DLL_EXPORT Font : public SR_UTILS_NS::IResource {
        using Super = SR_UTILS_NS::IResource;
        using FontLibrary = FT_Library;
        using FontFace = FT_Face;
        using StringType = std::u32string;
    private:
        Font();
        ~Font() override = default;

    public:
        static Font* Load(const SR_UTILS_NS::Path& rawPath);

        SR_NODISCARD bool HasColor() const noexcept { return m_hasColor; }
        SR_NODISCARD bool IsColorEmoji() const noexcept { return m_isColorEmoji; }

        SR_NODISCARD FT_Glyph GetGlyph(char32_t code, FT_Render_Mode renderMode, FT_Int32 charLoad, FT_Int32 glyphLoad) const;
        SR_NODISCARD FT_Glyph GetGlyph(char32_t code, FT_Render_Mode renderMode) const;

        SR_NODISCARD FT_Pos GetKerning(uint32_t leftCharCode, uint32_t rightCharCode) const;

        SR_NODISCARD SR_UTILS_NS::Path GetAssociatedPath() const override;

        bool SetPixelSizes(uint32_t w, uint32_t h);
        bool SetCharSize(uint32_t w, uint32_t h, uint32_t wRes, uint32_t hRes);

    protected:
        bool Unload() override;
        bool Load() override;

    private:
        FontLibrary m_library = nullptr;
        FontFace m_face = nullptr;

        bool m_hasColor = false;
        bool m_isColorEmoji = false;

    };
}

#endif //SR_ENGINE_FONT_H
