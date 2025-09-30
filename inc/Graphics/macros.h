#ifndef SR_ENGINE_GRAPHICS_MACROS_H
#define SR_ENGINE_GRAPHICS_MACROS_H

#include <Utils/macros.h>

#ifdef SR_GRAPHICS_DLL_EXPORTS
    #define SR_GRAPHICS_DLL_API SR_DLL_API_EXPORT
#else
    #define SR_GRAPHICS_DLL_API SR_DLL_API_IMPORT
#endif

#if 0 // enable if needed
    #define SR_RENDER_VALIDATION
#endif

#endif //SR_ENGINE_GRAPHICS_MACROS_H
