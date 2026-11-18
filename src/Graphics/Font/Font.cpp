//
// Created by Monika on 14.02.2022.
//

#include <Graphics/Font/Font.h>
#include <Graphics/Font/SDF.h>

#include <Utils/Resources/ResourceManager.h>
#include <Utils/FileSystem/VFS.h>
#include <Utils/Types/FlatHashMap.h>

#ifdef SR_RENDER_USE_HARFBUZZ
    #include <hb.h>
    #include <hb-ot.h>
#endif

#ifdef SR_USE_FREETYPE
    #include <freetype/include/freetype/ftglyph.h>
#endif

#ifdef SR_RENDER_USE_MSDFGEN
    #include <msdfgen.h>
    #include <ext/import-font.h>
    #include <core/Bitmap.h>
#endif

#include <Codegen/Font.generated.hpp>

namespace SR_GTYPES_NS {
    struct FontKerningContext {
    #ifdef SR_RENDER_USE_HARFBUZZ
        hb_font_t* pFont = nullptr;
        hb_buffer_t* pBuffer = nullptr;
        SR_HTYPES_NS::FlatHashMap<uint64_t, int32_t> pairs;

        ~FontKerningContext() {
            hb_buffer_destroy(pBuffer);
            hb_font_destroy(pFont);
        }

        int32_t GetPairAdjustment(uint32_t left, uint32_t right) {
            const uint64_t key = (static_cast<uint64_t>(left) << 32u) | right;
            if (auto&& pIt = pairs.find(key); pIt != pairs.end()) {
                return pIt->second;
            }

            SR_TRACY_ZONE;

            const uint32_t codepoints[] = { left, right };
            hb_feature_t features[] = {
                { HB_TAG('k', 'e', 'r', 'n'), 0, HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END },
                { HB_TAG('l', 'i', 'g', 'a'), 0, HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END },
                { HB_TAG('c', 'l', 'i', 'g'), 0, HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END },
                { HB_TAG('c', 'a', 'l', 't'), 0, HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END }
            };
            int32_t advances[2] = {};
            for (uint32_t pass = 0; pass < 2; ++pass) {
                hb_buffer_reset(pBuffer);
                hb_buffer_add_codepoints(pBuffer, codepoints, 2, 0, 2);
                hb_buffer_set_direction(pBuffer, HB_DIRECTION_LTR);
                hb_buffer_guess_segment_properties(pBuffer);
                features[0].value = pass;
                hb_shape(pFont, pBuffer, features, 4);

                uint32_t count = 0;
                auto&& pPositions = hb_buffer_get_glyph_positions(pBuffer, &count);
                auto&& pInfos = hb_buffer_get_glyph_infos(pBuffer, nullptr);
                hb_codepoint_t leftGlyph = 0;
                hb_codepoint_t rightGlyph = 0;
                hb_font_get_nominal_glyph(pFont, left, &leftGlyph);
                hb_font_get_nominal_glyph(pFont, right, &rightGlyph);
                if (count != 2 || pInfos[0].codepoint != leftGlyph || pInfos[1].codepoint != rightGlyph) {
                    pairs[key] = 0;
                    return 0;
                }
                advances[pass] = pPositions[0].x_advance + pPositions[1].x_advance;
            }
            const int32_t adjustment = advances[1] - advances[0];
            pairs[key] = adjustment;
            return adjustment;
        }
    #endif
    };

    Font::Font() = default;

    bool Font::Unload() {
        SR_TRACY_ZONE;

        delete m_pKerningContext;
        m_pKerningContext = nullptr;

        if (m_library) {
        #ifdef SR_USE_FREETYPE
            if (m_face) {
                FT_Done_Face(m_face);
                m_face = nullptr;
            }
            FT_Done_FreeType(m_library);
        #endif
            m_library = nullptr;
        }
        m_fontData.clear();

        return Super::Unload();
    }

    bool Font::Load() {
        SR_TRACY_ZONE;

        SR_UTILS_NS::Path path = CoreResLoader::GetResPath().Concat(GetResourcePath());

    #ifdef SR_USE_FREETYPE
        auto file = SR_UTILS_NS::VFS::Instance().OpenFile(path, SR_UTILS_NS::FileMode::Read);
        if (!file || !file.Read(m_fontData) || m_fontData.empty()) {
            SR_ERROR("Font::Load() : failed to read font!\n\tPath: {}", path);
            return false;
        }
        if (FT_Init_FreeType(&m_library)) {
            return false;
        }

        if (FT_New_Memory_Face(m_library, reinterpret_cast<const FT_Byte*>(m_fontData.data()),
            static_cast<FT_Long>(m_fontData.size()), 0, &m_face)) {
            FT_Done_FreeType(m_library);
            m_library = nullptr;
            m_fontData.clear();
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
    #ifdef SR_RENDER_USE_HARFBUZZ
        auto&& pBlob = hb_blob_create(m_fontData.data(), static_cast<uint32_t>(m_fontData.size()), HB_MEMORY_MODE_READONLY, nullptr, nullptr);
        auto&& pFace = hb_face_create(pBlob, 0);
        m_pKerningContext = new FontKerningContext();
        m_pKerningContext->pFont = hb_font_create(pFace);
        m_pKerningContext->pBuffer = hb_buffer_create();
        hb_ot_font_set_funcs(m_pKerningContext->pFont);
        const int32_t unitsPerEm = static_cast<int32_t>(hb_face_get_upem(pFace));
        hb_font_set_scale(m_pKerningContext->pFont, unitsPerEm, unitsPerEm);
        hb_face_destroy(pFace);
        hb_blob_destroy(pBlob);
    #endif
    #endif

        return Super::Load();
    }

    SR_UTILS_NS::Path Font::GetAssociatedPath() const {
        return SR_UTILS_NS::ResourceManager::Instance().GetResPath();
    }

    bool Font::SetPixelSizes(uint32_t w, uint32_t h) {
        const std::lock_guard<std::recursive_mutex> faceLock(m_faceMutex);
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
        const std::lock_guard<std::recursive_mutex> faceLock(m_faceMutex);
    #ifdef SR_USE_FREETYPE
        if (auto&& err = FT_Set_Char_Size(m_face, w, h, wRes, hRes)) {
            SR_ERROR("Font::SetCharSize() : failed to set char size!\n\tError: " + SRFreeTypeErrToString(err));
            return false;
        }
    #endif

        return true;
    }

    int32_t Font::GetKerning(uint32_t leftCharCode, uint32_t rightCharCode) const {
        const std::lock_guard<std::recursive_mutex> faceLock(m_faceMutex);
    #ifdef SR_USE_FREETYPE
    #ifdef SR_RENDER_USE_HARFBUZZ
        if (m_pKerningContext && m_face && m_face->size) {
            return static_cast<int32_t>(FT_MulFix(m_pKerningContext->GetPairAdjustment(leftCharCode, rightCharCode), m_face->size->metrics.x_scale));
        }
    #endif
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
    #else
        return 0;
    #endif
    }

#ifdef SR_USE_FREETYPE
    FT_Glyph Font::GetGlyph(char32_t code, FT_Render_Mode renderMode) const {
        if (HasColor()) {
            return GetGlyph(code, renderMode, FT_LOAD_RENDER, FT_LOAD_COLOR);
        }

        return GetGlyph(code, renderMode, FT_LOAD_RENDER, FT_LOAD_DEFAULT);
    }

    FT_Glyph Font::GetGlyph(char32_t code, FT_Render_Mode renderMode, FT_Int32 charLoad, FT_Int32 glyphLoad) const {
        const std::lock_guard<std::recursive_mutex> faceLock(m_faceMutex);
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

    bool Font::GenerateMSDFOrMTSDF(const char32_t code, SR_HTYPES_NS::FastMemoryArray<uint8_t, true, uint32_t>& out, uint32_t width, uint32_t height, float_t range, uint8_t padding, bool isMTSDF, uint32_t samplingPointSize) const {
        SR_TRACY_ZONE;

        if (width == 0 || height == 0) {
            return false;
        }

    #if defined(SR_RENDER_USE_MSDFGEN) && defined(SR_USE_FREETYPE)
        std::unique_lock<std::recursive_mutex> faceLock(m_faceMutex);
        if (!m_face) {
            SR_ERROR("Font::GenerateMSDFOrMTSDF() : font face is null!");
            return false;
        }

        if (samplingPointSize != 0 && FT_Set_Pixel_Sizes(m_face, 0, samplingPointSize)) {
            SR_ERROR("Font::GenerateMSDFOrMTSDF() : failed to set sampling size!");
            return false;
        }
        const FT_UInt glyphIndex = FT_Get_Char_Index(m_face, code);
        if (FT_Load_Glyph(m_face, glyphIndex, FT_LOAD_NO_BITMAP)) {
            SR_ERROR("Font::GenerateMSDFOrMTSDF() : FT_Load_Glyph failed for codepoint {}!", static_cast<uint32_t>(code));
            return false;
        }

        msdfgen::Shape shape;
        if (msdfgen::readFreetypeOutline(shape, &m_face->glyph->outline)) {
            SR_ERROR("Font::GenerateMSDFOrMTSDF() : readFreetypeOutline failed for codepoint {}!", static_cast<uint32_t>(code));
            return false;
        }

        shape.normalize();
        msdfgen::edgeColoringSimple(shape, 3.0);

        const msdfgen::Shape::Bounds bounds = shape.getBounds();

        const double glyphWidth  = bounds.r - bounds.l;
        const double glyphHeight = bounds.t - bounds.b;

        if (glyphWidth <= 0.0 || glyphHeight <= 0.0) {
            return false;
        }

        const double innerW = static_cast<double>(width) - 2.0 * padding;
        const double innerH = static_cast<double>(height) - 2.0 * padding;

        const double scale = std::min(innerW / glyphWidth, innerH / glyphHeight);

        const double scaledW = glyphWidth * scale;
        const double scaledH = glyphHeight * scale;
        const double offsetX = (innerW - scaledW) * 0.5;

        /// Та же логика ceil26d6, что в FontAsset::LoadGlyph для MTSDF (метрики квада совпадают с этим смещением).
        const auto ceil26d6Signed = [](FT_Pos v) -> double {
            if (v <= 0) {
                return static_cast<double>(v >> 6);
            }
            return static_cast<double>((v + static_cast<FT_Pos>(63)) >> 6);
        };
        const double coreBearingYPx = ceil26d6Signed(m_face->glyph->metrics.horiBearingY);
        faceLock.unlock();

        /// Строка baseline в выходной текстуре (сверху вниз): pad + расстояние от верха **core** до baseline в px при sampling size.
        /// Без вертикального letterboxing: иначе чернила «плавают» внутри квада относительно bearingY в TextMesh.
        const double baselineRowFromTop = SR_CLAMP(
            static_cast<double>(padding) + coreBearingYPx,
            0.0,
            static_cast<double>(height > 0 ? height - 1u : 0u)
        );
        /// В буфере msdfgen y идёт снизу вверх; после копирования flippedY верх картинки = y = height - 1.
        const double baselineYMsdf = static_cast<double>(height - 1u) - baselineRowFromTop;

        /// Projection: pixel = scale * (shape + translate). Для контура baseline в декартовых координатах FT — y = 0.
        const double translateY = baselineYMsdf / scale;

        /// msdfgen::Projection: translate в координатах контура (см. core/Projection.cpp).
        msdfgen::Vector2 translate(
            (static_cast<double>(padding) + offsetX) / scale - bounds.l,
            translateY
        );

        msdfgen::Projection proj(msdfgen::Vector2(scale, scale), translate);

        // Sign correction can introduce new interpolation artifacts; correct them afterwards.
        msdfgen::MSDFGeneratorConfig generatorConfig;
        generatorConfig.errorCorrection.mode = msdfgen::ErrorCorrectionConfig::DISABLED;
        msdfgen::MSDFGeneratorConfig postErrorCorrectionConfig;
        postErrorCorrectionConfig.errorCorrection.distanceCheckMode = msdfgen::ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE;

        if (isMTSDF) {
            msdfgen::Bitmap<float, 4> mtsdf(width, height);

            {
                SR_TRACY_ZONE_N("msdfgen::generateMTSDF");
                msdfgen::generateMTSDF(mtsdf, shape, proj, range, generatorConfig);
                msdfgen::distanceSignCorrection(mtsdf, shape, proj, 0.5);
                msdfgen::msdfErrorCorrection(mtsdf, shape, proj, range, postErrorCorrectionConfig);
            }

            out.resize(width * height * 4);
            for (uint32_t y = 0; y < height; ++y) {
                uint32_t flippedY = height - 1 - y;
                for (uint32_t x = 0; x < width; ++x) {
                    const float* v = mtsdf(x, y);
                    uint32_t dst = (flippedY * width + x) * 4;
                    out[dst + 0] = static_cast<uint8_t>(SR_CLAMP(v[0] * 255.0f, 0.0f, 255.0f));
                    out[dst + 1] = static_cast<uint8_t>(SR_CLAMP(v[1] * 255.0f, 0.0f, 255.0f));
                    out[dst + 2] = static_cast<uint8_t>(SR_CLAMP(v[2] * 255.0f, 0.0f, 255.0f));
                    out[dst + 3] = static_cast<uint8_t>(SR_CLAMP(v[3] * 255.0f, 0.0f, 255.0f));
                }
            }
        }
        else {
            msdfgen::Bitmap<float, 3> msdf(width, height);

            {
                SR_TRACY_ZONE_N("msdfgen::generateMSDF");
                msdfgen::generateMSDF(msdf, shape, proj, range, generatorConfig);
                msdfgen::distanceSignCorrection(msdf, shape, proj, 0.5);
                msdfgen::msdfErrorCorrection(msdf, shape, proj, range, postErrorCorrectionConfig);
            }

            out.resize(width * height * 4);
            for (uint32_t y = 0; y < height; ++y) {
                uint32_t flippedY = height - 1 - y;
                for (uint32_t x = 0; x < width; ++x) {
                    const float* v = msdf(x, y);
                    uint32_t dst = (flippedY * width + x) * 4;
                    out[dst + 0] = static_cast<uint8_t>(SR_CLAMP(v[0] * 255.0f, 0.0f, 255.0f));
                    out[dst + 1] = static_cast<uint8_t>(SR_CLAMP(v[1] * 255.0f, 0.0f, 255.0f));
                    out[dst + 2] = static_cast<uint8_t>(SR_CLAMP(v[2] * 255.0f, 0.0f, 255.0f));
                    out[dst + 3] = 255;
                }
            }
        }

        return true;
    #else
        SR_WARN("Font::GenerateMSDFOrMTSDF() : msdfgen or free-type is not enabled, cannot generate MSDF/MTSDF!");
        return false;
    #endif
    }
}
