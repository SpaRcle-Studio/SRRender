#ifdef SR_RENDER_USE_NATIVE_WAYLAND
extern "C" {
    #include <fractional-scale-v1-client-protocol.h>
    #include <pointer-constraints-unstable-v1.h>
    #include <relative-pointer-unstable-v1.h>
    #include <xdg-decoration-unstable-v1.h>
    #include <xdg-shell-client-protocol.h>

    #include "xdg-decoration-unstable-v1.c"

    #ifdef WL_PRIVATE
        #undef WL_PRIVATE
    #endif

    #include "xdg-shell-client-protocol.c"

    #ifdef WL_PRIVATE
        #undef WL_PRIVATE
    #endif

    #include "fractional-scale-v1-client-protocol.c"

    #ifdef WL_PRIVATE
        #undef WL_PRIVATE
    #endif

    #include "pointer-constraints-unstable-v1.c"

    #ifdef WL_PRIVATE
        #undef WL_PRIVATE
    #endif

    #include "relative-pointer-unstable-v1.c"
}
#endif
