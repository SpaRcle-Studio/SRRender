//
// Created by Monika on 14.02.2022.
//

#ifndef SR_ENGINE_FONT_H
#define SR_ENGINE_FONT_H

#include <Graphics/Font/FreeType.h>

#include <Utils/Resources/IResource.h>

namespace SR_GTYPES_NS {
    class SR_GRAPHICS_DLL_API Font : public SR_UTILS_NS::IResource {
        SR_CLASS()
        using Super = SR_UTILS_NS::IResource;
    #ifdef SR_USE_FREETYPE
        using FontLibrary = FT_Library;
        using FontFace = FT_Face;
    #else
        using FontLibrary = void*;
        using FontFace = void*;
    #endif
        using StringType = std::u32string;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<Font>;

    public:
        Font();
        ~Font() override = default;

    public:
        SR_NODISCARD bool IsAllowedToRevive() const override { return true; }

        SR_NODISCARD bool HasColor() const noexcept { return m_hasColor; }
        SR_NODISCARD bool IsColorEmoji() const noexcept { return m_isColorEmoji; }

    #ifdef SR_USE_FREETYPE
        SR_NODISCARD FT_Glyph GetGlyph(char32_t code, FT_Render_Mode renderMode, FT_Int32 charLoad, FT_Int32 glyphLoad) const;
        SR_NODISCARD FT_Glyph GetGlyph(char32_t code, FT_Render_Mode renderMode) const;
        SR_NODISCARD FT_Pos GetKerning(uint32_t leftCharCode, uint32_t rightCharCode) const;
    #endif

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
