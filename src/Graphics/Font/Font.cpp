//
// Created by Monika on 14.02.2022.
//

#include <Graphics/Font/Font.h>
#include <Graphics/Font/SDF.h>

#include <Utils/Resources/ResourceManager.h>

#ifdef SR_USE_FREETYPE
    #include <freetype/include/freetype/ftglyph.h>
#endif

#ifdef SR_RENDER_USE_MSDFGEN
    #include <msdfgen.h>
    #include <msdfgen/ext/import-font.h>
    #include <core/Bitmap.h>
#endif

#include <Codegen/Font.generated.hpp>

namespace SR_GTYPES_NS {
    Font::Font() = default;

    bool Font::Unload() {
        SR_TRACY_ZONE;

        if (m_library) {
        #ifdef SR_USE_FREETYPE
            FT_Done_FreeType(m_library);
        #endif
            m_library = nullptr;
        }

    #ifdef SR_RENDER_USE_MSDFGEN
        if (m_MSDFFontHandle) {
            msdfgen::destroyFont((msdfgen::FontHandle*)m_MSDFFontHandle);
            m_MSDFFontHandle = nullptr;
        }
        if (m_MSDFFreeTypeHandle) {
            msdfgen::deinitializeFreetype((msdfgen::FreetypeHandle*)m_MSDFFreeTypeHandle);
            m_MSDFFreeTypeHandle = nullptr;
        }
    #endif

        return Super::Unload();
    }

    bool Font::Load() {
        SR_TRACY_ZONE;

        SR_UTILS_NS::Path&& path = SR_UTILS_NS::Path(GetResourceId());
        if (!path.IsAbs()) {
            path = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(path);
        }

    #ifdef SR_RENDER_USE_MSDFGEN
        m_MSDFFreeTypeHandle = msdfgen::initializeFreetype();
        if (!m_MSDFFreeTypeHandle) {
            SR_ERROR("Font::Load() : failed to initialize free-type for msdfgen!\n\tPath: {}", path);
            return false;
        }

        m_MSDFFontHandle = msdfgen::loadFont((msdfgen::FreetypeHandle*)m_MSDFFreeTypeHandle, path.c_str());
        if (!m_MSDFFontHandle) {
            SR_ERROR("Font::Load() : failed to load font for msdfgen!\n\tPath: {}", path);
            return false;
        }
    #endif

    #ifdef SR_USE_FREETYPE
        FT_Init_FreeType(&m_library);

        if (FT_New_Face(m_library, path.c_str(), 0, &m_face)) {
            SR_ERROR("Font::Load() : failed to load free-type font!\n\tPath: {}", path);
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
    #endif

        return Super::Load();
    }

    SR_UTILS_NS::Path Font::GetAssociatedPath() const {
        return SR_UTILS_NS::ResourceManager::Instance().GetResPath();
    }

    bool Font::SetPixelSizes(uint32_t w, uint32_t h) {
    #ifdef SR_USE_FREETYPE
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
    #endif

        return true;
    }

    bool Font::SetCharSize(uint32_t w, uint32_t h, uint32_t wRes, uint32_t hRes) {
    #ifdef SR_USE_FREETYPE
        if (auto&& err = FT_Set_Char_Size(m_face, w, h, wRes, hRes)) {
            SR_ERROR("Font::SetCharSize() : failed to set char size!\n\tError: " + SRFreeTypeErrToString(err));
            return false;
        }
    #endif

        return true;
    }

#ifdef SR_USE_FREETYPE
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
#endif

    bool Font::GenerateMSDF(const char32_t code, SR_HTYPES_NS::FastMemoryArray<uint8_t, true, uint32_t>& out, uint32_t width, uint32_t height, float_t range, uint8_t padding) const {
        return GenerateMSDFOrMTSDF(code, out, width, height, range, padding, false);
    }

    bool Font::GenerateMTSDF(const char32_t code, SR_HTYPES_NS::FastMemoryArray<uint8_t, true, uint32_t>& out, uint32_t width, uint32_t height, float_t range, uint8_t padding) const {
        return GenerateMSDFOrMTSDF(code, out, width, height, range, padding, true);
    }

    bool Font::GenerateMSDFOrMTSDF(const char32_t code, SR_HTYPES_NS::FastMemoryArray<uint8_t, true, uint32_t>& out, uint32_t width, uint32_t height, float_t range, uint8_t padding, bool isMTSDF) const {
        SR_TRACY_ZONE;

        if (width == 0 || height == 0) {
            return false;
        }

    #ifdef SR_RENDER_USE_MSDFGEN
        if (!m_MSDFFontHandle) {
            SR_ERROR("Font::GenerateMSDFOrMTSDF() : font handle is null!");
            return false;
        }

        msdfgen::Shape shape;
        msdfgen::loadGlyph(shape, (msdfgen::FontHandle*)m_MSDFFontHandle, code);

        shape.normalize();
        msdfgen::edgeColoringSimple(shape, 3.0);

        msdfgen::Vector2 scale(1.0, 1.0);
        msdfgen::Projection proj(scale, msdfgen::Vector2(padding, padding));

        if (isMTSDF) {
            msdfgen::Bitmap<float, 4> mtsdf(width, height);

            {
                SR_TRACY_ZONE_N("msdfgen::generateMTSDF");
                msdfgen::generateMTSDF(mtsdf, shape, proj, range);
            }

            out.resize(width * height * 4);
            for (uint32_t y = 0; y < height; ++y) {
                for (uint32_t x = 0; x < width; ++x) {
                    const float* v = mtsdf(x, y);
                    out[(y * width + x) * 4 + 0] = static_cast<uint8_t>(SR_CLAMP(v[0] * 255.0f, 0.0f, 255.0f));
                    out[(y * width + x) * 4 + 1] = static_cast<uint8_t>(SR_CLAMP(v[1] * 255.0f, 0.0f, 255.0f));
                    out[(y * width + x) * 4 + 2] = static_cast<uint8_t>(SR_CLAMP(v[2] * 255.0f, 0.0f, 255.0f));
                    out[(y * width + x) * 4 + 3] = static_cast<uint8_t>(SR_CLAMP(v[3] * 255.0f, 0.0f, 255.0f));
                }
            }
        }
        else {
            msdfgen::Bitmap<float, 3> msdf(width, height);

            {
                SR_TRACY_ZONE_N("msdfgen::generateMSDF");
                msdfgen::generateMSDF(msdf, shape, proj, range);
            }

            out.resize(width * height * 4);
            for (uint32_t y = 0; y < height; ++y) {
                for (uint32_t x = 0; x < width; ++x) {
                    const float* v = msdf(x, y);
                    out[(y * width + x) * 4 + 0] = static_cast<uint8_t>(SR_CLAMP(v[0] * 255.0f, 0.0f, 255.0f));
                    out[(y * width + x) * 4 + 1] = static_cast<uint8_t>(SR_CLAMP(v[1] * 255.0f, 0.0f, 255.0f));
                    out[(y * width + x) * 4 + 2] = static_cast<uint8_t>(SR_CLAMP(v[2] * 255.0f, 0.0f, 255.0f));
                    out[(y * width + x) * 4 + 3] = 255;
                }
            }
        }

        return true;
    #else
        SR_WARN("Font::GenerateMSDFOrMTSDF() : msdfgen is not supported!");
        return false;
    #endif
    }
}