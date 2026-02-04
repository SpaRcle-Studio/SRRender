#ifdef SR_RENDER_USE_NATIVE_WAYLAND
    extern "C" {
        #include <xdg-shell-client-protocol.h>
        #include <xdg-decoration-unstable-v1.h>

        #include "xdg-decoration-unstable-v1.c"
        #ifdef WL_PRIVATE
            #undef WL_PRIVATE
        #endif
        #include "xdg-shell-client-protocol.c"
    }
#endif
