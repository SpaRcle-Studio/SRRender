//
// Created by monika on 2/4/26.
//

#ifdef SR_RENDER_USE_NATIVE_WAYLAND

#include <Graphics/Window/WaylandWindow.h>

#include <Utils/Profile/TracyContext.h>
#include <Utils/Platform/Platform.h>
#include <Utils/Input/KeyCodes.h>
#include <Utils/Input/InputSystem.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <poll.h>

#define DECORATIONS_BAR_SIZE 8
#define DECORATIONS_TOPBAR_SIZE 32
#define DECORATIONS_BUTTON_SIZE 32

namespace SR_GRAPH_NS {
    namespace Details {
        std::string_view GetErrnoDescription(int error) {
            switch (error) {
                case EACCES:       return "EACCES: Permission denied";
                case EEXIST:       return "EEXIST: File exists";
                case EINVAL:       return "EINVAL: Invalid argument";
                case EMFILE:       return "EMFILE: Too many open files";
                case ENFILE:       return "ENFILE: File table overflow";
                case ENOENT:       return "ENOENT: No such file or directory";
                case ENOSPC:       return "ENOSPC: No space left on device";
                case EPERM:        return "EPERM: Operation not permitted";
                case EROFS:        return "EROFS: Read-only file system";
                case ETXTBSY:      return "ETXTBSY: Text file busy";
                case ENOMEM:       return "ENOMEM: Out of memory";
                case EFBIG:        return "EFBIG: File too large";
                case EIO:          return "EIO: I/O error";
                case EPIPE:        return "EPIPE: Broken pipe";
                case EINTR:        return "EINTR: Interrupted system call";
                case EAGAIN:       return "EAGAIN: Resource temporarily unavailable";
                case ENOSYS:       return "ENOSYS: Function not implemented";
                case EBADF:        return "EBADF: Bad file descriptor";
                case EDEADLK:      return "EDEADLK: Resource deadlock would occur";
                case ENAMETOOLONG: return "ENAMETOOLONG: File name too long";
                case ENOLCK:       return "ENOLCK: No record locks available";
                case ENOTEMPTY:    return "ENOTEMPTY: Directory not empty";
                case ELOOP:        return "ELOOP: Too many symbolic links encountered";
                case ENOMSG:       return "ENOMSG: No message of desired type";
                case EIDRM:        return "EIDRM: Identifier removed";
                case ECHRNG:       return "ECHRNG: Channel number out of range";
                case EL2NSYNC:     return "EL2NSYNC: Level 2 not synchronized";
                case EL3HLT:       return "EL3HLT: Level 3 halted";
                case EL3RST:       return "EL3RST: Level 3 reset";
                case ELNRNG:       return "ELNRNG: Link number out of range";
                case EUNATCH:      return "EUNATCH: Protocol driver not attached";
                case ENOCSI:       return "ENOCSI: No CSI structure available";
                case EL2HLT:       return "EL2HLT: Level 2 halted";
                case EBADE:        return "EBADE: Invalid exchange";
                case EBADR:        return "EBADR: Invalid request descriptor";
                case EXFULL:       return "EXFULL: Exchange full";
                case ENOANO:       return "ENOANO: No anode";
                case EBADRQC:      return "EBADRQC: Invalid request code";
                case EBADSLT:      return "EBADSLT: Invalid slot";
                case EBFONT:       return "EBFONT: Bad font file format";
                case ENOSTR:       return "ENOSTR: Device not a stream";
                case ENODATA:      return "ENODATA: No data available";
                case ETIME:        return "ETIME: Timer expired";
                case ENOSR:        return "ENOSR: Out of streams resources";
                case ENONET:       return "ENONET: Machine is not on the network";
                case ENOPKG:       return "ENOPKG: Package not installed";
                case EREMOTE:      return "EREMOTE: Object is remote";
                case ENOLINK:      return "ENOLINK: Link has been severed";
                case EADV:         return "EADV: Advertise error";
                case ESRMNT:       return "ESRMNT: Srmount error";
                case ECOMM:        return "ECOMM: Communication error on send";
                case EPROTO:       return "EPROTO: Protocol error";
                case EMULTIHOP:    return "EMULTIHOP: Multihop attempted";
                case EDOTDOT:      return "EDOTDOT: RFS specific error";
                case EBADMSG:      return "EBADMSG: Not a data message";
                case EOVERFLOW:    return "EOVERFLOW: Value too large for defined data type";
                case ENOTUNIQ:     return "ENOTUNIQ: Name not unique on network";
                case EBADFD:       return "EBADFD: File descriptor in bad state";
                case EREMCHG:      return "EREMCHG: Remote address changed";
                default:           return "Unknown error";
            }
        }

        uint32_t ToColor(char r, char g, char b, char a) {
            uint32_t result = 0;
            char* c = (char*)&result;
            c[0] = b;
            c[1] = g;
            c[2] = r;
            c[3] = a;
            return result;
        }

        void registry_add_object(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
        void registry_remove_object(void *data, wl_registry *registry, uint32_t name);
        wl_registry_listener registry_listener = {&registry_add_object, &registry_remove_object};

        void seat_capabilities(void *data, wl_seat *seat, uint32_t capabilities);
        wl_seat_listener seat_listener = {&seat_capabilities};

        void xdg_wm_base_ping(void *data, struct xdg_wm_base *base, uint32_t serial);
        xdg_wm_base_listener xdg_wm_base_listener = {.ping = xdg_wm_base_ping};

        void xdg_surface_handle_configure(void *data, xdg_surface *xdg_surface, uint32_t serial);
        xdg_surface_listener xdg_surface_listener = {.configure = xdg_surface_handle_configure};

        void xdg_toplevelconfigure_bounds(void *data, xdg_toplevel *xdg_toplevel, int32_t width, int32_t height);
        void xdg_toplevel_handle_configure(void *data, xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states);
        void xdg_toplevel_handle_close(void *data, xdg_toplevel *xdg_toplevel);
        xdg_toplevel_listener xdg_toplevel_listener = {.configure = xdg_toplevel_handle_configure, .close = xdg_toplevel_handle_close, .configure_bounds = xdg_toplevelconfigure_bounds};

        void pointer_enter(void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
        void pointer_leave(void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface);
        void pointer_motion(void *data, wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y);
        void pointer_button(void *data, wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
        void pointer_axis(void *data, wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
        wl_pointer_listener pointer_listener = {&pointer_enter, &pointer_leave, &pointer_motion, &pointer_button, &pointer_axis};

        void output_scale(void* pData, wl_output* pOutput, const int32_t scale) {
            SR_LOG("output_scale() : found output {}, set scale to {}", static_cast<void*>(pOutput), scale);
            static_cast<WaylandWindow*>(pData)->FindOutput(pOutput).scale = scale;
        }

        static void output_geometry(void*, wl_output*, int32_t, int32_t, int32_t, int32_t, int32_t, const char*, const char*, int32_t) { }
        static void output_mode(void*, wl_output*, uint32_t, int32_t, int32_t, int32_t) { }
        static void output_done(void*, wl_output*) { }

        static const wl_output_listener output_listener = { .geometry = output_geometry, .mode = output_mode, .done = output_done, .scale = output_scale };

        void keyboard_keymap(void* pData, wl_keyboard*, uint32_t format, int fd, uint32_t size) {
            auto&& pWindow = static_cast<WaylandWindow*>(pData);

            if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
                close(fd);
                return;
            }

            char* map = (char*)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

            xkb_keymap* pXkbKeymap = xkb_keymap_new_from_string(pWindow->GetXkbContext(), map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

            munmap(map, size);
            close(fd);

            pWindow->SetXkbKeymap(pXkbKeymap);
            pWindow->SetXkbState(xkb_state_new(pXkbKeymap));
        }

        void keyboard_modifiers(void* pData, wl_keyboard*, uint32_t serial, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group) {
            auto&& pWindow = static_cast<WaylandWindow*>(pData);

            xkb_state_update_mask(pWindow->GetXkbState(), depressed, latched, locked, 0, 0, group);

            auto&& keyboardState = SR_PLATFORM_NS::GetOverriddenKeyboardState();
            if (!keyboardState) {
                keyboardState = SR_PLATFORM_NS::KeyboardState();
            }

            keyboardState->Set(SR_UTILS_NS::KeyCode::LCtrl, xkb_state_mod_name_is_active(pWindow->GetXkbState(), XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE));
            keyboardState->Set(SR_UTILS_NS::KeyCode::LShift, xkb_state_mod_name_is_active(pWindow->GetXkbState(), XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE));
            keyboardState->Set(SR_UTILS_NS::KeyCode::LAlt, xkb_state_mod_name_is_active(pWindow->GetXkbState(), XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE));
            keyboardState->Set(SR_UTILS_NS::KeyCode::Super, xkb_state_mod_name_is_active(pWindow->GetXkbState(), XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE));
        }

        void keyboard_enter(void* pData, wl_keyboard*, uint32_t serial, wl_surface*, wl_array* keys) {
            // do nothing...
        }

        void keyboard_leave(void* pData, wl_keyboard*, uint32_t serial, wl_surface*) {
            // do nothing...
        }

        void keyboard_key(void* pData, wl_keyboard*, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
            auto&& pWindow = static_cast<WaylandWindow*>(pData);

            auto&& keyboardState = SR_PLATFORM_NS::GetOverriddenKeyboardState();
            if (!keyboardState) {
                keyboardState = SR_PLATFORM_NS::KeyboardState();
            }

            //const xkb_keysym_t* syms;
            //int numSyms = xkb_state_key_get_syms(pWindow->GetXkbState(), key + 8, &syms);
            //if (numSyms > 0) {
            //    for (int i = 0; i < numSyms; ++i) {
            //        auto keysym = syms[i];
            //        auto keyCode = SR_UTILS_NS::KeyCodeFromXkbKeysym(keysym);
            //        if (keyCode != SR_UTILS_NS::KeyCode::None) {
            //            keyboardState->Set(keyCode, state == WL_KEYBOARD_KEY_STATE_PRESSED);
            //        }
            //    }
            //}

            const bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

            const uint32_t xkbKey = key + 8;

            xkb_state_update_key(pWindow->GetXkbState(), xkbKey, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);

            if (auto keyCode = SR_UTILS_NS::KeyCodeFromEvdev(key); keyCode != SR_UTILS_NS::KeyCode::None) {
                keyboardState->Set(keyCode, pressed);
            }

            if (pressed) {
                char utf8[64] = {};
                if (const int n = xkb_state_key_get_utf8(pWindow->GetXkbState(), xkbKey, utf8, sizeof(utf8)); n > 0) {
                    SR_UTILS_NS::InputTextEvent event;
                    event.SetText(std::string_view(utf8, static_cast<size_t>(n)));
                    event.pSource = pWindow;
                    SR_UTILS_NS::Input::Instance().AddTextEvent(std::move(event));
                }
            }

            SR_PLATFORM_NS::SetOverriddenKeyboardState(keyboardState);
        }

        void keyboard_repeat_info(void* pData, wl_keyboard*, int32_t rate, int32_t delay) {
            // do nothing...
        }

        static const wl_keyboard_listener keyboard_listener = {
            .keymap = keyboard_keymap,
            .enter  = keyboard_enter,
            .leave  = keyboard_leave,
            .key    = keyboard_key,
            .modifiers = keyboard_modifiers,
            .repeat_info = keyboard_repeat_info
        };

        void registry_add_object(void* pData, wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
            SR_TRACY_ZONE;

            auto&& pWindow = static_cast<WaylandWindow*>(pData);

            if (strcmp(interface, wl_output_interface.name) == 0) {
                wl_output* pOutput = static_cast<wl_output*>(wl_registry_bind(registry, name, &wl_output_interface, 2));
                wl_output_add_listener(pOutput, &output_listener, pData);
                pWindow->AddOutput(WaylandWindow::Output{.name = name, .output = pOutput, .scale = 1 });
            }
            else if (strcmp(interface, wp_fractional_scale_manager_v1_interface.name) == 0) {
                pWindow->SetFractionalScaleManager(static_cast<wp_fractional_scale_manager_v1*>(wl_registry_bind(registry, name, &wp_fractional_scale_manager_v1_interface, 1)));
            }
            else if (!strcmp(interface, wl_compositor_interface.name)) {
                pWindow->SetCompositor(static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1)));
            }
            else if (strcmp(interface, wl_subcompositor_interface.name) == 0) {
                pWindow->SetSubCompositor(static_cast<wl_subcompositor *>(wl_registry_bind(registry, name, &wl_subcompositor_interface, 1)));
            }
            else if (strcmp(interface, "xdg_wm_base") == 0) {
                pWindow->SetXdgWMBase(static_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, std::min(version, 2U))));
                xdg_wm_base_add_listener(pWindow->GetXdgWMBase(), &xdg_wm_base_listener, pData);
            }
            else if (!strcmp(interface,wl_seat_interface.name)) {
                auto&& pSeat = static_cast<struct wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, 1));
                pWindow->SetSeat(pSeat);
                wl_seat_add_listener(pSeat, &seat_listener, pData);
            }
            else if (strcmp(interface, wl_shm_interface.name) == 0) {
                auto&& pShm = static_cast<struct wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
                pWindow->SetShm(pShm);
                //pWindow->SetCursorTheme(wl_cursor_theme_load(NULL, 32, pShm));
            }
            else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
                pWindow->SetXdgWMBase(static_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, SR_MIN(version, 2))));
            }
            else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
                pWindow->SetDecorationManager(static_cast<zxdg_decoration_manager_v1*>(wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1)));
            }
        }

        void registry_remove_object(void *data, wl_registry *registry, uint32_t name) {
            // do nothing...
        }

        void seat_capabilities(void* pData, wl_seat* pSeat, uint32_t capabilities) {
            auto&& pWindow = static_cast<WaylandWindow*>(pData);
            if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
                wl_pointer* pointer = wl_seat_get_pointer(pSeat);
                wl_pointer_add_listener(pointer, &pointer_listener, pData);
                pWindow->SetCursorSurface(wl_compositor_create_surface(pWindow->GetCompositor()));
                pWindow->SetCursorPointer(pointer);
            }
            if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
                auto&& pKeyboard = wl_seat_get_keyboard(pSeat);
                pWindow->SetKeyboard(pKeyboard);
                wl_keyboard_add_listener(pKeyboard, &keyboard_listener, pData);
            }
        }

        void pointer_enter(void* pData, wl_pointer *pointer, uint32_t serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
        {
            //mouseHoverSurface = surface;
            //mouseHoverSerial = serial;
            //SetMousePos(x, y);
            //SetCursor(pointer, serial, "left_ptr");

            auto&& pWindow = static_cast<WaylandWindow*>(pData);

            if (!pWindow->GetCursorTheme()) {
                pWindow->SetCursorTheme(wl_cursor_theme_load(NULL, 24, pWindow->GetShm()));
            }

            wl_cursor* cursor = wl_cursor_theme_get_cursor(pWindow->GetCursorTheme(), "left_ptr");
            wl_cursor_image* image = cursor->images[0];
            wl_buffer* buffer = wl_cursor_image_get_buffer(image);

            wl_surface_attach(pWindow->GetCursorSurface(), buffer, 0, 0);
            wl_surface_damage(pWindow->GetCursorSurface(), 0, 0, image->width, image->height);
            wl_surface_commit(pWindow->GetCursorSurface());

            wl_pointer_set_cursor(pointer, serial, pWindow->GetCursorSurface(), image->hotspot_x, image->hotspot_y);
        }

        void pointer_leave(void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface)
        {
            //mouseHoverSurface = NULL;
            //mouseHoverSerial = -1;
        }

        void pointer_motion(void* pData, wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
        {
            const float_t x = wl_fixed_to_double(sx);
            const float_t y = wl_fixed_to_double(sy);

            auto&& mouseState = SR_PLATFORM_NS::GetOverriddenMouseState();
            if (!mouseState) {
                mouseState = SR_PLATFORM_NS::GetMouseState();
            }
            mouseState->position = { x, y };
            SR_PLATFORM_NS::SetOverriddenMouseState(mouseState);
        }

        void pointer_button(void* data, wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
        {
            auto&& mouseState = SR_PLATFORM_NS::GetOverriddenMouseState();
            if (!mouseState) {
                mouseState = SR_PLATFORM_NS::GetMouseState();
            }

            bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);

            switch (button) {
                case BTN_LEFT:   mouseState->buttonStates[0] = pressed; break;
                case BTN_RIGHT:  mouseState->buttonStates[1] = pressed; break;
                case BTN_MIDDLE: mouseState->buttonStates[2] = pressed; break;
                case BTN_SIDE:   mouseState->buttonStates[3] = pressed; break;
                case BTN_EXTRA:  mouseState->buttonStates[4] = pressed; break;
                default:
                    break;
            }

            SR_PLATFORM_NS::SetOverriddenMouseState(mouseState);
        }

        void pointer_axis(void* pData, wl_pointer*, uint32_t time, uint32_t axis, wl_fixed_t value) {
            auto&& pWindow = static_cast<WaylandWindow*>(pData);

            const float_t v = wl_fixed_to_double(value);

            switch (axis) {
                case WL_POINTER_AXIS_VERTICAL_SCROLL:
                    if (auto&& callback = pWindow->GetScrollCallback())                   {
                        callback(pWindow, 0, -v);
                    }
                    break;
                case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
                    if (auto&& callback = pWindow->GetScrollCallback()) {
                        callback(pWindow, v, 0);
                    }
                    break;
                default:
                    break;
            }
        }

        void xdg_surface_handle_configure(void* data, xdg_surface *xdg_surface, uint32_t serial) {
            xdg_surface_ack_configure(xdg_surface, serial);
            auto&& pWindow = static_cast<WaylandWindow*>(data);

            if (pWindow->IsUseClientDecorations()) {
                wl_surface_commit(pWindow->GetClientSurface());
            }
            wl_surface_commit(pWindow->GetSurface());
            wl_display_flush(pWindow->GetDisplay());
        }

        void xdg_toplevelconfigure_bounds(void*, xdg_toplevel*, int32_t, int32_t)
        {
            // do nothing...
        }

        void xdg_toplevel_handle_configure(void *data, xdg_toplevel*, int32_t width, int32_t height, wl_array *states) {
            auto&& pWindow = static_cast<WaylandWindow*>(data);

            pWindow->SetWaitingForConfigure(false);

            int activated = 0;
            int maximized = 0;
            int fullscreen = 0;
            int resizing = 0;
            int floating = 1;
            const void* state = NULL;
            wl_array_for_each(state, states) {
                switch (*static_cast<const uint32_t*>(state)) {
                    case XDG_TOPLEVEL_STATE_ACTIVATED: activated = 1; break;
                    case XDG_TOPLEVEL_STATE_RESIZING: resizing = 1; break;
                    case XDG_TOPLEVEL_STATE_MAXIMIZED: maximized = 1; break;
                    case XDG_TOPLEVEL_STATE_FULLSCREEN: fullscreen = 1; break;

                    case XDG_TOPLEVEL_STATE_TILED_LEFT:
                    case XDG_TOPLEVEL_STATE_TILED_RIGHT:
                    case XDG_TOPLEVEL_STATE_TILED_TOP:
                    case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
                        floating = 0;
                        break;
                }
            }

            // manage maximized state
            int currentMaximized = pWindow->IsMaximized();
            if (!pWindow->IsMaximized() && maximized) {
                pWindow->SetMaximized(true);
                SR_LOG("xdg_toplevel_handle_configure() : window maximized");
            }
            else if (pWindow->IsMaximized() && floating) {
                pWindow->SetMaximized(false);
                SR_LOG("xdg_toplevel_handle_configure() : window unmaximized");
            }

            pWindow->SetPendingSize({width, height});

            // resize window
            /*if (activated || resizing || maximized || fullscreen || currentMaximized != pWindow->IsMaximized()) {
                if (width > 0 && height > 0 && (pWindow->GetCompositeWidth() != width || pWindow->GetCompositeHeight() != height)) {
                    SR_LOG("xdg_toplevel_handle_configure() : resizing window to {}x{}", width, height);

                    int clientWidth = width;
                    int clientHeight = height;
                    if (pWindow->IsUseClientDecorations()) {
                        clientWidth = width - (DECORATIONS_BAR_SIZE * 2);
                        clientHeight = height - (DECORATIONS_BAR_SIZE + DECORATIONS_TOPBAR_SIZE);
                    }
                    pWindow->InternalSetWindowSize(clientWidth, clientHeight);

                    if (pWindow->IsUseClientDecorations()) {
                        pWindow->ResizeSurfaceBuffer(&pWindow->GetClientSurfaceBuffer(), pWindow->GetClientSurface());
                        wl_surface_damage(pWindow->GetClientSurface(), 0, 0, pWindow->GetClientSurfaceBuffer().width, pWindow->GetClientSurfaceBuffer().height);
                        wl_surface_commit(pWindow->GetClientSurface());
                    }

                    pWindow->ResizeSurfaceBuffer(&pWindow->GetSurfaceBuffer(), pWindow->GetSurface());
                    // TODO: if (useClientDecorations)
                    //    DrawButtons();
                    wl_surface_damage(pWindow->GetSurface(), 0, 0, pWindow->GetSurfaceBuffer().width, pWindow->GetSurfaceBuffer().height);
                    wl_surface_commit(pWindow->GetSurface());

                    wl_display_flush(pWindow->GetDisplay());
                }
            }*/
        }

        void surface_enter(void* pData, wl_surface*, wl_output* output) {
            auto&& pWindow = static_cast<WaylandWindow*>(pData);
            pWindow->AddEnteredOutput(output);
            pWindow->RecalculateScale();
        }

        void surface_leave(void* pData, wl_surface*, wl_output* output) {
            auto&& pWindow = static_cast<WaylandWindow*>(pData);
            pWindow->RemoveEnteredOutput(output);
            pWindow->RecalculateScale();
        }

        static const wl_surface_listener surface_listener = {
            .enter = surface_enter,
            .leave = surface_leave
        };

        void xdg_toplevel_handle_close(void* pData, xdg_toplevel*) {
            SR_LOG("xdg_toplevel_handle_close() : window close requested by compositor");
            static_cast<WaylandWindow*>(pData)->SetValid(false);
            static_cast<WaylandWindow*>(pData)->SetClosed(true);
        }

        void xdg_wm_base_ping(void*, xdg_wm_base* base, uint32_t serial) {
            xdg_wm_base_pong(base, serial);
        }

        void decoration_handle_configure(void* data, zxdg_toplevel_decoration_v1*, uint32_t mode) {
            static_cast<WaylandWindow*>(data)->SetDecorationMode(static_cast<zxdg_toplevel_decoration_v1_mode>(mode));
            switch (mode) {
                case ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE:
                    SR_LOG("decoration_handle_configure() : client-side decorations requested by compositor");
                    break;
                case ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE:
                    SR_LOG("decoration_handle_configure() : server-side decorations requested by compositor");
                    break;
                default:
                    SRHalt("decoration_handle_configure() : unknown decoration mode {}", static_cast<int32_t>(mode));
                    break;
            }
        }

        static const zxdg_toplevel_decoration_v1_listener decoration_listener = {.configure = decoration_handle_configure};

        bool CreateSurfaceBuffer(WaylandWindow::SurfaceBuffer* buffer, wl_shm* shm, struct wl_surface* surface, const char* name, uint32_t color) {
            SR_LOG("WaylandWindow::CreateSurfaceBuffer() : creating surface buffer ({}x{}, name={})", buffer->width, buffer->height, name ? name : "unnamed");
            // get buffer sizes
            int oldSize = buffer->size;
            buffer->stride = buffer->width * sizeof(uint32_t);
            buffer->size = buffer->height * buffer->stride;

            // alloc name if needed
            if (name) {
                if (buffer->name) {
                    free(buffer->name);
                }
                size_t nameSize = strlen(name);
                buffer->name = (char*)malloc(nameSize);
                memcpy(buffer->name, name, nameSize);
            }

            // only create new file if needed
            if (buffer->fd < 0) {
                buffer->fd = shm_open(buffer->name, O_RDWR | O_CREAT | O_TRUNC, 0600);
                if (buffer->fd < 0 || errno == EEXIST) {
                    SR_ERROR("WaylandWindow::CreateSurfaceBuffer() : failed to create shm file!");
                    return false;
                }
            }

            // set file size
            if (buffer->size > oldSize)// only increase file size or we can get buffer access violations in pool
            {
                int result = ftruncate(buffer->fd, buffer->size);
                if (result < 0 || errno == EINTR) {
                    SR_ERROR("WaylandWindow::CreateSurfaceBuffer() : failed to set shm file size!");
                    return false;
                }
            }

            // map memory
            buffer->pixels = (uint32_t*)mmap(NULL, buffer->size, PROT_READ | PROT_WRITE, MAP_SHARED, buffer->fd, 0);
            shm_unlink(buffer->name);// call after mmap according to docs

            /// memset(buffer->pixels, color, buffer->width * buffer->height * sizeof(uint32_t));// clear to color
            for (int i = 0; i < buffer->width * buffer->height; ++i) {
                buffer->pixels[i] = color;
            }

            // create pool
            buffer->pool = wl_shm_create_pool(shm, buffer->fd, buffer->size);
            buffer->buffer = wl_shm_pool_create_buffer(buffer->pool, 0, buffer->width, buffer->height, buffer->stride, WL_SHM_FORMAT_XRGB8888);
            buffer->color = color;

            wl_surface_attach(surface, buffer->buffer, 0, 0);
            SR_LOG("WaylandWindow::CreateSurfaceBuffer() : created surface buffer ({}x{}, fd={}, size={})", buffer->width, buffer->height, buffer->fd, buffer->size);
            return true;
        }

        void fractional_preferred_scale(void* pData, wp_fractional_scale_v1*, const uint32_t scale) {
            auto&& pWindow = static_cast<WaylandWindow*>(pData);
            pWindow->SetFractionalScale(scale / 120.0f);
        }

        static const wp_fractional_scale_v1_listener fractional_listener = {
            .preferred_scale = fractional_preferred_scale
        };
    }

    void WaylandWindow::InternalSetWindowSize(int width, int height)
    {
        m_internalWidth = width;
        m_internalHeight = height;

        if (m_useClientDecorations) {
            m_compositeWidth = m_internalWidth + (DECORATIONS_BAR_SIZE * 2);
            m_compositeHeight = m_internalHeight + (DECORATIONS_BAR_SIZE + DECORATIONS_TOPBAR_SIZE);
            m_surfaceBuffer.width = m_compositeWidth;
            m_surfaceBuffer.height = m_compositeHeight;
            m_clientSurfaceBuffer.width = width;
            m_clientSurfaceBuffer.height = height;

            // CSD rects
            m_clientRectDragTopBar = SR_MATH_NS::IRect(0, 0, m_compositeWidth, DECORATIONS_TOPBAR_SIZE);

            m_clientRectResizeLeftBar = SR_MATH_NS::IRect(0, 0, DECORATIONS_BAR_SIZE, m_compositeHeight);
            m_clientRectResizeRightBar = SR_MATH_NS::IRect(m_compositeWidth - DECORATIONS_BAR_SIZE, 0, DECORATIONS_BAR_SIZE, m_compositeHeight);
            m_clientRectResizeBottomBar = SR_MATH_NS::IRect(0, m_compositeHeight - DECORATIONS_BAR_SIZE, m_compositeWidth, DECORATIONS_BAR_SIZE);
            m_clientRectResizeTopBar = SR_MATH_NS::IRect(0, 0, m_compositeWidth, DECORATIONS_BAR_SIZE);

            m_clientRectResizeTopLeft = SR_MATH_NS::IRect(0, 0, DECORATIONS_BAR_SIZE, DECORATIONS_BAR_SIZE);
            m_clientRectResizeTopRight = SR_MATH_NS::IRect(m_compositeWidth - DECORATIONS_BAR_SIZE, 0, DECORATIONS_BAR_SIZE, DECORATIONS_BAR_SIZE);
            m_clientRectResizeBottomLeft = SR_MATH_NS::IRect(0, m_compositeHeight - DECORATIONS_BAR_SIZE, DECORATIONS_BAR_SIZE, DECORATIONS_BAR_SIZE);
            m_clientRectResizeBottomRight = SR_MATH_NS::IRect(m_compositeWidth - DECORATIONS_BAR_SIZE, m_compositeHeight - DECORATIONS_BAR_SIZE, DECORATIONS_BAR_SIZE, DECORATIONS_BAR_SIZE);

            int x = m_compositeWidth - (24 + 4);
            m_clientRectButtonClose = SR_MATH_NS::IRect(x, 4, 24, 24);
            x -= 24 + 4;
            m_clientRectButtonMax = SR_MATH_NS::IRect(x, 4, 24, 24);
            x -= 24 + 4;
            m_clientRectButtonMin = SR_MATH_NS::IRect(x, 4, 24, 24);
        }
        else {
            m_compositeWidth = width;
            m_compositeHeight = height;
            m_surfaceBuffer.width = width;
            m_surfaceBuffer.height = height;
            m_clientSurfaceBuffer.width = -1;
            m_clientSurfaceBuffer.height = -1;
        }

        SR_LOG("WaylandWindow::InternalSetWindowSize() : \n\tInternal size set to {}x{}\n\tComposite size set to {}x{}"
               "\n\tClient surface buffer size set to {}x{}\n\tSurface buffer size set to {}x{}\n\tDecorations: {}",
            m_internalWidth, m_internalHeight, m_compositeWidth, m_compositeHeight, m_clientSurfaceBuffer.width, m_clientSurfaceBuffer.height,
            m_surfaceBuffer.width, m_surfaceBuffer.height, m_useClientDecorations ? "enabled" : "disabled"
        );
    }

    bool WaylandWindow::ResizeSurfaceBuffer(SurfaceBuffer* pBuffer, wl_surface* pSurface) {
        SR_LOG("WaylandWindow::ResizeSurfaceBuffer() : resizing surface buffer to {}x{}", pBuffer->width, pBuffer->height);

        // pre-dispose old buffers
        munmap(pBuffer->pixels, pBuffer->size);
        wl_shm_pool_destroy(pBuffer->pool);
        wl_buffer* oldBuffer = pBuffer->buffer;// dispose after new buffer is created

        // create new buffer
        const bool result = Details::CreateSurfaceBuffer(pBuffer, m_shm, pSurface, NULL, pBuffer->color);

        // post-dispose old buffer
        wl_buffer_destroy(oldBuffer);

        if (m_resizeCallback) {
            m_resizeCallback(this, pBuffer->width, pBuffer->height);
        }

        return result;
    }

    auto wait_for_data_on_fd(int filde, int waitms) -> bool {
        struct pollfd fds;
        fds.fd = filde;
        fds.events = POLLIN;

        poll(&fds, 1, waitms);

        if ((fds.revents & (POLLERR | POLLNVAL | POLLHUP)) != 0) {
            throw std::system_error(EIO, std::generic_category());
        }

        return (fds.revents & POLLIN) != 0;
    }

    void WaylandWindow::RecalculateScale() {
        int max_scale = 1;

        for (wl_output* pOutput : m_enteredOutputs) {
            max_scale = std::max(max_scale, FindOutput(pOutput).scale);
        }

        m_scale = max_scale;

        SR_LOG("WaylandWindow::RecalculateScale() : recalculated scale to {}", m_scale);
    }

    bool WaylandWindow::DoWaylandPollEvents() {
        while (wl_display_prepare_read(m_display) != 0)
            wl_display_dispatch_pending(m_display);
        wl_display_flush(m_display);
        wl_display_read_events(m_display);
        wl_display_dispatch_pending(m_display);

        /*bool isEagain = false;
        if (const int ret = wl_display_prepare_read(m_display); ret != 0) {
            const int err = errno;
            if (err == EAGAIN) {
                isEagain = true;
            }
            else {
                SR_ERROR("WaylandWindow::DoWaylandPollEvents() : wl_display_prepare_read failed with error \"{}\"!", Details::GetErrnoDescription(err));
                return false;
            }
        }

        if (!isEagain) {
            wl_display_flush(m_display);
            wl_display_read_events(m_display);
        }

        if (const int ret = wl_display_dispatch_pending(m_display); ret < 0) {
            SR_ERROR("WaylandWindow::DoWaylandPollEvents() : wl_display_dispatch_pending failed with error \"{}\"!", Details::GetErrnoDescription(errno));
            m_isClosed = true;
            m_isValid = false;
            return false;
        }*/

        /*const auto wl_fd = wl_display_get_fd(m_display);
        bool in_event = false;

        // prepare to read wayland events
        while (wl_display_prepare_read(m_display) != 0) {
            wl_display_dispatch_pending(m_display);
        }
        wl_display_flush(m_display);

        try {
            constexpr int waitms = 100;
            in_event = wait_for_data_on_fd(wl_fd, waitms);
        }
        catch (const std::system_error &err) {
            SR_ERROR("WaylandWindow::DoWaylandPollEvents() : wait_for_data_on_fd failed with error \"{}\"!", err.what());
            m_isClosed = true;
            m_isValid = false;
            return false;
        }

        if (in_event) {
            wl_display_read_events(m_display);
            wl_display_dispatch_pending(m_display);
        }
        else {
            wl_display_cancel_read(m_display);
        }*/

        return true;
    }

    void WaylandWindow::ThreadFunction() {
        SR_LOG("WaylandWindow::ThreadFunction() : thread started.");

        while (m_isValid && !m_isClosed) {
            DoWaylandPollEvents();
        }

        SR_LOG("WaylandWindow::ThreadFunction() : thread finished.");
    }

    bool WaylandWindow::Initialize(const std::string& name, const SR_MATH_NS::IVector2& position, const SR_MATH_NS::UVector2& size, bool fullScreen, bool resizable) {
        m_display = wl_display_connect(NULL);
        if (!m_display) {
            SR_ERROR("WaylandWindow::Initialize() : failed to connect to Wayland display!");
            return false;
        }

        /// que registry
        wl_registry* pRegistry = wl_display_get_registry(m_display);
        wl_registry_add_listener(pRegistry, &Details::registry_listener, this);
        wl_display_roundtrip(m_display);
        m_useClientDecorations = (m_decorationManager == nullptr) ? 1 : 0;
        if (m_useClientDecorations) {
            SR_LOG("WaylandWindow::Initialize() : using client-side decorations.");
        }
        else {
            SR_LOG("WaylandWindow::Initialize() : using server-side decorations.");
        }

        m_xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

        /// create window
        m_surfaceBuffer.fd = -1;
        m_clientSurfaceBuffer.fd = -1;

        InternalSetWindowSize(320, 240);

        m_surface = wl_compositor_create_surface(m_compositor);
        m_xdgSurface = xdg_wm_base_get_xdg_surface(m_xdgWMBase, m_surface);
        m_xdgToplevel = xdg_surface_get_toplevel(m_xdgSurface);

        wl_surface_add_listener(m_surface, &Details::surface_listener, this);

        if (m_fractionalScaleManager) {
            m_fractionalScale = wp_fractional_scale_manager_v1_get_fractional_scale(m_fractionalScaleManager, m_surface);
            wp_fractional_scale_v1_add_listener(m_fractionalScale, &Details::fractional_listener, this);
        }

        xdg_surface_add_listener(m_xdgSurface, &Details::xdg_surface_listener, this);
        xdg_toplevel_add_listener(m_xdgToplevel, &Details::xdg_toplevel_listener, this);

        xdg_toplevel_set_title(m_xdgToplevel, name.c_str());
        xdg_toplevel_set_app_id(m_xdgToplevel, "SREngine");
        xdg_toplevel_set_min_size(m_xdgToplevel, 300, 300);

        /// get server-side decorations
        if (!m_useClientDecorations && m_decorationManager) {
            m_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(m_decorationManager, m_xdgToplevel);
            zxdg_toplevel_decoration_v1_add_listener(m_decoration, &Details::decoration_listener, this);
        }

        // surface buffers
        uint32_t color = m_useClientDecorations ? Details::ToColor(127, 127, 127, 255) : Details::ToColor(50, 50, 50, 255);
        if (Details::CreateSurfaceBuffer(&m_surfaceBuffer, GetShm(), m_surface, "WaylandClientWindow_Decorations", color) != 1) {
            SR_ERROR("WaylandWindow::Initialize() : failed to create surface buffer!");
            return false;
        }

        wl_surface_commit(m_surface);

        if (m_useClientDecorations) {
            m_clientSurface = wl_compositor_create_surface(m_compositor);
            m_clientSubSurface = wl_subcompositor_get_subsurface(m_subCompositor, m_clientSurface, m_surface);
            wl_subsurface_set_desync(m_clientSubSurface);
            wl_subsurface_set_position(m_clientSubSurface, DECORATIONS_BAR_SIZE, DECORATIONS_TOPBAR_SIZE);
            if (Details::CreateSurfaceBuffer(&m_clientSurfaceBuffer, GetShm(), m_clientSurface, "WaylandClientWindow_Client", Details::ToColor(255, 255, 255, 255)) != 1) {
                SR_ERROR("WaylandWindow::Initialize() : failed to create client surface buffer!");
                return false;
            }
            /// TODO: DrawButtons();
        }

        // finalize surfaces
        if (m_useClientDecorations) {
            wl_surface_damage(m_clientSurface, 0, 0, m_clientSurfaceBuffer.width, m_clientSurfaceBuffer.height);
            wl_surface_commit(m_clientSurface);
        }

        wl_surface_damage(m_surface, 0, 0, m_surfaceBuffer.width, m_surfaceBuffer.height);
        wl_surface_commit(m_surface);
        wl_display_flush(m_display);

        m_isValid = true;
        m_isFocused = true;

        m_waitingForConfigure = true;
        SR_LOG("WaylandWindow::Initialize() : waiting for Wayland configuration...");
        while (m_waitingForConfigure) {
            wl_display_roundtrip(m_display);
        }
        SR_LOG("WaylandWindow::Initialize() : Wayland configuration received, window is now valid.");

        return true;
    }

    WaylandWindow::Output& WaylandWindow::FindOutput(wl_output* pOutput) {
        for (auto& output : m_outputs) {
            if (output.output == pOutput) {
                return output;
            }
        }
        SRHalt("WaylandWindow::FindOutput() : failed to find output for wl_output {}!", static_cast<void*>(pOutput));
        static Output dummy;
        return dummy;
    }

    void WaylandWindow::PollEvents() {
        Super::PollEvents();

        DoWaylandPollEvents();

        if (m_pendingSize && !m_pendingSize->HasZero() && !m_pendingSize->HasNegative() && m_lastSize != *m_pendingSize) {
            m_lastSize = *m_pendingSize;
            m_pendingSize.reset();

            const SR_MATH_NS::IVector2 surfaceSize = m_lastSize * GetScale();

            SR_LOG("WaylandWindow::PollEvents() : applying pending configure with size {}x{}...", surfaceSize.x, surfaceSize.y);

            InternalSetWindowSize(surfaceSize.x, surfaceSize.y);

            if (m_useClientDecorations) {
                ResizeSurfaceBuffer(&m_clientSurfaceBuffer, m_clientSurface);
                wl_surface_damage(m_clientSurface, 0, 0, m_clientSurfaceBuffer.width, m_clientSurfaceBuffer.height);
                wl_surface_commit(m_clientSurface);
            }

            ResizeSurfaceBuffer(&m_surfaceBuffer, m_surface);

            //wl_surface_damage(GetSurface(), 0, 0, GetSurfaceBuffer().width, GetSurfaceBuffer().height);
            //wl_surface_commit(GetSurface());

            //wl_display_flush(GetDisplay());
        }
    }

    void WaylandWindow::Close() {
        SR_LOG("WaylandWindow::Close() : closing window...");

        Super::Close();

        if (m_thread && m_thread->Joinable()) {
            m_thread->Join();
            m_thread->Free();
        }

        SR_LOG("WaylandWindow::Close() : destroying Wayland resources...");

        if (m_pXkbState) {
            xkb_state_unref(m_pXkbState);
            m_pXkbState = nullptr;
        }

        if (m_pXkbKeymap) {
            xkb_keymap_unref(m_pXkbKeymap);
            m_pXkbKeymap = nullptr;
        }

        if (m_xkbContext) {
            xkb_context_unref(m_xkbContext);
            m_xkbContext = nullptr;
        }

        // shutdown
        if (m_useClientDecorations) {
            munmap(m_clientSurfaceBuffer.pixels, m_clientSurfaceBuffer.size);
            wl_shm_pool_destroy(m_clientSurfaceBuffer.pool);
            wl_buffer_destroy(m_clientSurfaceBuffer.buffer);
        }

        munmap(m_surfaceBuffer.pixels, m_surfaceBuffer.size);
        wl_shm_pool_destroy(m_surfaceBuffer.pool);
        wl_buffer_destroy(m_surfaceBuffer.buffer);

        if(m_xdgWMBase) {
            xdg_toplevel_destroy(m_xdgToplevel);
            xdg_surface_destroy(m_xdgSurface);
        }

        if (m_decoration) {
            zxdg_toplevel_decoration_v1_destroy(m_decoration);
        }

        if (m_clientSubSurface)  {
            wl_surface_destroy(m_clientSurface);
            wl_subsurface_destroy(m_clientSubSurface);
        }
        wl_surface_destroy(m_surface);
        wl_display_disconnect(m_display);

        SR_LOG("WaylandWindow::Close() : window closed.");
    }
}

#endif