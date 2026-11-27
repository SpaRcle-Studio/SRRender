//
// Created by Monika on 11.11.2025.
//

#ifndef SR_ENGINE_GRAPHICS_STD_INCLUDE_H
#define SR_ENGINE_GRAPHICS_STD_INCLUDE_H

#include <Graphics/macros.h>

#include <Utils/stdInclude.h>

namespace SR_GRAPH_NS {
    class RenderContext;

    constexpr char EMSCRIPTEN_CANVAS_ID[] = "#srengine-canvas";
}

#if defined(SR_ENGINE_USE_MODULAR_BUILD) && !defined(SR_ENGINE_CODEGEN_CLANG_PARSE_MODE)
#if !defined(SR_GRAPHICS_DLL_EXPORTS) || defined(SR_LINUX)
namespace SR_HTYPES_NS {
    extern template class SR_GRAPHICS_DLL_API SharedPtr<SR_GRAPH_NS::RenderContext>;
}
#endif
#endif

#endif //SR_ENGINE_GRAPHICS_STD_INCLUDE_H
