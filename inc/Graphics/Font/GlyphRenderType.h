//
// Created by Monika on 30.04.2026.
//

#ifndef SR_ENGINE_GRAPHICS_GLYPH_RENDER_TYPE_H
#define SR_ENGINE_GRAPHICS_GLYPH_RENDER_TYPE_H

#include <Graphics/stdInclude.h>

#include <Utils/Common/Enumerations.h>

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(GlyphRenderType, uint8_t,
        SDF,
        MSDF,
        MTSDF,
        Bitmap,
        ColorBitmap
    )
}

#endif //SR_ENGINE_GRAPHICS_GLYPH_RENDER_TYPE_H
