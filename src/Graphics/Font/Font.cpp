//
// Created by Monika on 14.02.2022.
//

#include <Graphics/Font/Font.h>
#include <Graphics/Font/SDF.h>

#include <freetype/include/freetype/ftglyph.h>

namespace SR_GTYPES_NS {
    Font::Font()
        : Super(SR_COMPILE_TIME_CRC32_TYPE_NAME(Font))
    { }

    Font* Font::Load(const SR_UTILS_NS::Path& rawPath) {
        SR_TRACY_ZONE;
        SR_GLOBAL_LOCK

        SR_UTILS_NS::Path&& path = SR_UTILS_NS::Path(rawPath).RemoveSubPath(SR_UTILS_NS::ResourceManager::Instance().GetResPath());

        if (auto&& pResource = SR_UTILS_NS::ResourceManager::Instance().Find<Font>(path)) {
            return pResource;
        }

        auto&& pResource = new Font();

        pResource->SetId(path.ToStringRef(), false /** auto register */);

        if (!pResource->Reload()) {
            SR_ERROR("Font::Load() : failed to load font! \n\tPath: " + path.ToString());
            pResource->DeleteResource();
            return nullptr;
        }

        /// отложенная ручная регистрация
        SR_UTILS_NS::ResourceManager::Instance().RegisterResource(pResource);

        return pResource;
    }

    bool Font::Unload() {
        SR_TRACY_ZONE;

        if (m_library) {
            FT_Done_FreeType(m_library);
            m_library = nullptr;
        }

        return Super::Unload();
    }

    bool Font::Load() {
        SR_TRACY_ZONE;

        FT_Init_FreeType(&m_library);

        SR_UTILS_NS::Path&& path = SR_UTILS_NS::Path(GetResourceId());
        if (!path.IsAbs()) {
            path = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(path);
        }

        if (FT_New_Face(m_library, path.c_str(), 0, &m_face)) {
            SR_ERROR("Font::Load() : failed to load free-type font! \n\tPath: " + path.ToString());
            return false;
        }

        if (FT_Select_Charmap(m_face, FT_ENCODING_UNICODE)) {
            SR_ERROR("Font::Load() : failed to set char map!");
            return false;
        }

        m_hasColor = FT_HAS_COLOR(m_face);

        static const uint32_t tag = FT_MAKE_TAG('C', 'B', 'D', 'T');
        FT_ULong length = 0;
        FT_Load_Sfnt_Table(m_face, tag, 0, nullptr, &length);
        m_isColorEmoji = length > 0;

        return Super::Load();
    }

    SR_UTILS_NS::Path Font::GetAssociatedPath() const {
        return SR_UTILS_NS::ResourceManager::Instance().GetResPath();
    }

    bool Font::SetPixelSizes(uint32_t w, uint32_t h) {
        if (IsColorEmoji()) {
            if (m_face->num_fixed_sizes == 0) {
                SR_ERROR("Font::SetPixelSizes() : num fixes sizes is zero!");
                return false;
            }

            int32_t best_match = 0;
            int32_t diff = std::abs(static_cast<int32_t>(h - m_face->available_sizes[0].width));
            for (int32_t i = 1; i < m_face->num_fixed_sizes; ++i) {
                int32_t ndiff = std::abs(static_cast<int32_t>(h - m_face->available_sizes[i].width));
                if (ndiff < diff) {
                    best_match = i;
                    diff = ndiff;
                }
            }

            if (FT_Select_Size(m_face, best_match)) {
                SR_ERROR("Font::SetPixelSizes() : failed to select size!");
                return false;
            }
        }
        else {
            if (FT_Set_Pixel_Sizes(m_face, w, h)) {
                SR_ERROR("Font::SetPixelSizes() : failed to set pixel sizes!");
                return false;
            }
        }

        return true;
    }

    bool Font::SetCharSize(uint32_t w, uint32_t h, uint32_t wRes, uint32_t hRes) {
        if (auto&& err = FT_Set_Char_Size(m_face, w, h, wRes, hRes)) {
            SR_ERROR("Font::SetCharSize() : failed to set char size!\n\tError: " + SRFreeTypeErrToString(err));
            return false;
        }

        return true;
    }

    FT_Pos Font::GetKerning(uint32_t leftCharCode, uint32_t rightCharCode) const {
        if (!FT_HAS_KERNING(m_face)) {
            return 0;
        }

        /// Получаем индекс левого символа
        FT_UInt leftIndex = FT_Get_Char_Index(m_face, leftCharCode);
        /// Получаем индекс правого символа
        FT_UInt rightIndex = FT_Get_Char_Index(m_face, rightCharCode);
        /// Здесь будет хранится кернинг в формате 26.6
        FT_Vector delta;
        /// Получаем кернинг для двух символов
        FT_Get_Kerning(m_face, leftIndex, rightIndex, FT_KERNING_DEFAULT, &delta);

        return delta.x;
    }

    FT_Glyph Font::GetGlyph(char32_t code, FT_Render_Mode renderMode) const {
        if (HasColor()) {
            return GetGlyph(code, renderMode, FT_LOAD_RENDER, FT_LOAD_COLOR);
        }

        return GetGlyph(code, renderMode, FT_LOAD_RENDER, FT_LOAD_DEFAULT);
    }

    FT_Glyph Font::GetGlyph(char32_t code, FT_Render_Mode renderMode, FT_Int32 charLoad, FT_Int32 glyphLoad) const {
         FT_Glyph glyph = nullptr;

        if (FT_Load_Char(m_face, code, charLoad)) {
            SR_WARN("Font::GetGlyph() : failed to load char!");
            return nullptr;
        }

        FT_UInt glyph_index = FT_Get_Char_Index(m_face, code);

        if (FT_Load_Glyph(m_face, glyph_index, glyphLoad)) {
            SR_WARN("Font::GetGlyph() : failed to load glyph!");
            return nullptr;
        }

        if (FT_Render_Glyph(m_face->glyph, renderMode)) {
            SR_WARN("Font::GetGlyph() : failed to render glyph!");
            return nullptr;
        }

        if (FT_Get_Glyph(m_face->glyph, &glyph)) {
            SR_WARN("Font::GetGlyph() : failed to get glyph!");
            return nullptr;
        }

        return glyph;
    }
}