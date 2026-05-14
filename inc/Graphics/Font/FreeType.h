//
// Created by mantsurov-n on 02.11.2022.
//

#ifndef SR_ENGINE_FREETYPE_H
#define SR_ENGINE_FREETYPE_H

#include <Graphics/stdInclude.h>
#include <Utils/Types/FastMemoryArray.h>

#ifdef SR_USE_FREETYPE

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftbbox.h>
#include <freetype/ftglyph.h>
#include <freetype/tttables.h>
#include <freetype/src/truetype/ttobjs.h>

namespace SR_GRAPH_NS {
    SR_MAYBE_UNUSED static std::string FreeTypeErrToString(FT_Error err) {
#undef FTERRORS_H_
#define FT_ERRORDEF(e, v, s)  case e: return s;
#define FT_ERROR_START_LIST     switch (err) {
#define FT_ERROR_END_LIST       }

#include FT_ERRORS_H
        return "(Unknown error)";
    }

    SR_MAYBE_UNUSED static void FTUnusedFunctions() {
    #ifdef SR_LINUX
    #else
        tt_glyphzone_done(nullptr);
        tt_glyphzone_new(nullptr, 0, 0, nullptr);
        tt_size_init(nullptr);
        tt_size_done(nullptr);
        tt_size_run_fpgm(nullptr, false);
        tt_size_run_prep(nullptr, false);
        tt_size_ready_bytecode(nullptr, false);
        tt_size_reset(nullptr);
        tt_driver_init(nullptr);
        tt_driver_done(nullptr);
        tt_slot_init(nullptr);
    #endif
    }

    SR_GRAPHICS_DLL_API extern void FreeTypeGenerateSDF(const FT_Bitmap& bmp, SR_HTYPES_NS::FastMemoryArray<float>& out, uint32_t width, uint32_t height, float_t range);
}

#define SRFreeTypeErrToString(err) (FreeTypeErrToString(err))

#else
    #define SRFreeTypeErrToString(err)
#endif

#endif //SR_ENGINE_FREETYPE_H
