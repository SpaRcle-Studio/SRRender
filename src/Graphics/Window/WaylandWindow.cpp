//
// Created by monika on 2/4/26.
//

#include <Graphics/Window/WaylandWindow.h>

#include <fcntl.h>
#include <sys/mman.h>

#define DECORATIONS_BAR_SIZE 8
#define DECORATIONS_TOPBAR_SIZE 32
#define DECORATIONS_BUTTON_SIZE 32

namespace SR_GRAPH_NS {
    namespace Details {
        uint32_t ToColor(char r, char g, char b, char a) {
            uint32_t result = 0;
            char* c = (char*)&result;
            c[0] = b;
            c[1] = g;
            c[2] = r;
            c[3] = a;
            return result;
        }

        int ResizeSurfaceBuffer(WaylandWindow::SurfaceBuffer* buffer, wl_shm* shm, wl_surface* surface);

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

        void registry_add_object(void* pData, wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
            auto&& pWindow = static_cast<WaylandWindow*>(pData);

            if (!strcmp(interface, wl_compositor_interface.name)) {
                pWindow->SetCompositor(static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, 1)));
            }
            else if (strcmp(interface, wl_subcompositor_interface.name) == 0) {
                pWindow->SetSubCompositor(static_cast<wl_subcompositor *>(wl_registry_bind(registry, name, &wl_subcompositor_interface, 1)));
            }
            else if (!strcmp(interface,wl_seat_interface.name)) {
                auto&& pSeat = static_cast<struct wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, 1));
                pWindow->SetSeat(pSeat);
                wl_seat_add_listener(pSeat, &seat_listener, pData);
            }
            else if (strcmp(interface, wl_shm_interface.name) == 0) {
                auto&& pShm = static_cast<struct wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
                pWindow->SetShm(pShm);
                pWindow->SetCursorTheme(wl_cursor_theme_load(NULL, 32, pShm));
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
                wl_pointer *pointer = wl_seat_get_pointer(pSeat);
                wl_pointer_add_listener(pointer, &pointer_listener, pData);
                pWindow->SetCursorSurface(wl_compositor_create_surface(pWindow->GetCompositor()));
            }
        }

        void pointer_enter(void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
        {
            //mouseHoverSurface = surface;
            //mouseHoverSerial = serial;
            //SetMousePos(x, y);
            //SetCursor(pointer, serial, "left_ptr");
        }

        void pointer_leave(void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface)
        {
            //mouseHoverSurface = NULL;
            //mouseHoverSerial = -1;
        }

        void pointer_motion(void *data, wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
        {
            //SetMousePos(x, y);
        }

        void pointer_button(void *data, wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
        {
            /*if (!useClientDecorations) return;
            if (button == BTN_LEFT)
            {
                if (mouseHoverSurface == window->surface)
                {
                    if (state == WL_POINTER_BUTTON_STATE_RELEASED)
                    {
                        // buttons
                        if (WithinRect(window->clientRect_ButtonClose, mouseX, mouseY))
                        {
                            running = 0;
                        }
                        else if (WithinRect(window->clientRect_ButtonMax, mouseX, mouseY))
                        {
                            if (!window->isMaximized)
                            {
                                xdg_toplevel_set_maximized(window->xdg_toplevel);
                                //xdg_toplevel_set_fullscreen(window->xdg_toplevel, NULL);
                            }
                            else
                            {
                                xdg_toplevel_unset_maximized(window->xdg_toplevel);
                                //xdg_toplevel_unset_fullscreen(window->xdg_toplevel);
                            }
                        }
                        else if (WithinRect(window->clientRect_ButtonMin, mouseX, mouseY))
                        {
                            xdg_toplevel_set_minimized(window->xdg_toplevel);
                        }
                    }
                    else if (state == WL_POINTER_BUTTON_STATE_PRESSED)
                    {
                        // drag
                        if
                        (
                            !WithinRect(window->clientRect_ButtonClose, mouseX, mouseY) && !WithinRect(window->clientRect_ButtonMax, mouseX, mouseY) && !WithinRect(window->clientRect_ButtonMin, mouseX, mouseY) &&
                            !WithinRect(window->clientRect_Resize_BottomBar, mouseX, mouseY) && !WithinRect(window->clientRect_Resize_TopBar, mouseX, mouseY) && !WithinRect(window->clientRect_Resize_LeftBar, mouseX, mouseY) && !WithinRect(window->clientRect_Resize_RightBar, mouseX, mouseY) &&
                            !WithinRect(window->clientRect_Resize_TopLeft, mouseX, mouseY) && !WithinRect(window->clientRect_Resize_TopRight, mouseX, mouseY) && !WithinRect(window->clientRect_Resize_BottomLeft, mouseX, mouseY) && !WithinRect(window->clientRect_Resize_BottomRight, mouseX, mouseY)
                        )
                        {
                            if (WithinRect(window->clientRect_Drag_TopBar, mouseX, mouseY)) xdg_toplevel_move(window->xdg_toplevel, seat, serial);
                        }
                        // resize corners
                        else if (WithinRect(window->clientRect_Resize_TopLeft, mouseX, mouseY))
                        {
                            xdg_toplevel_resize(window->xdg_toplevel, seat, serial, XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT);
                        }
                        else if (WithinRect(window->clientRect_Resize_TopRight, mouseX, mouseY))
                        {
                            xdg_toplevel_resize(window->xdg_toplevel, seat, serial, XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT);
                        }
                        else if (WithinRect(window->clientRect_Resize_BottomLeft, mouseX, mouseY))
                        {
                            xdg_toplevel_resize(window->xdg_toplevel, seat, serial, XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT);
                        }
                        else if (WithinRect(window->clientRect_Resize_BottomRight, mouseX, mouseY))
                        {
                            xdg_toplevel_resize(window->xdg_toplevel, seat, serial, XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT);
                        }
                        // resize edges
                        else if (WithinRect(window->clientRect_Resize_BottomBar, mouseX, mouseY))
                        {
                            xdg_toplevel_resize(window->xdg_toplevel, seat, serial, XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM);
                        }
                        else if (WithinRect(window->clientRect_Resize_TopBar, mouseX, mouseY))
                        {
                            xdg_toplevel_resize(window->xdg_toplevel, seat, serial, XDG_TOPLEVEL_RESIZE_EDGE_TOP);
                        }
                        else if (WithinRect(window->clientRect_Resize_LeftBar, mouseX, mouseY))
                        {
                            xdg_toplevel_resize(window->xdg_toplevel, seat, serial, XDG_TOPLEVEL_RESIZE_EDGE_LEFT);
                        }
                        else if (WithinRect(window->clientRect_Resize_RightBar, mouseX, mouseY))
                        {
                            xdg_toplevel_resize(window->xdg_toplevel, seat, serial, XDG_TOPLEVEL_RESIZE_EDGE_RIGHT);
                        }
                    }
                }
            }*/
        }

        void pointer_axis(void*, wl_pointer*, uint32_t, uint32_t, wl_fixed_t) {
            // TODO
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

            // resize window
            if (activated || resizing || maximized || fullscreen || currentMaximized != pWindow->IsMaximized()) {
                if (width > 0 && height > 0 && (pWindow->GetCompositeWidth() != width || pWindow->GetCompositeHeight() != height)) {
                    int clientWidth = width;
                    int clientHeight = height;
                    if (pWindow->IsUseClientDecorations()) {
                        clientWidth = width - (DECORATIONS_BAR_SIZE * 2);
                        clientHeight = height - (DECORATIONS_BAR_SIZE + DECORATIONS_TOPBAR_SIZE);
                    }
                    pWindow->InternalSetWindowSize(clientWidth, clientHeight);

                    if (pWindow->IsUseClientDecorations()) {
                        ResizeSurfaceBuffer(&pWindow->GetClientSurfaceBuffer(), pWindow->GetShm(), pWindow->GetClientSurface());
                        wl_surface_damage(pWindow->GetClientSurface(), 0, 0, pWindow->GetClientSurfaceBuffer().width, pWindow->GetClientSurfaceBuffer().height);
                        wl_surface_commit(pWindow->GetClientSurface());
                    }

                    ResizeSurfaceBuffer(&pWindow->GetSurfaceBuffer(), pWindow->GetShm(), pWindow->GetSurface());
                    // TODO: if (useClientDecorations)
                    //    DrawButtons();
                    wl_surface_damage(pWindow->GetSurface(), 0, 0, pWindow->GetSurfaceBuffer().width, pWindow->GetSurfaceBuffer().height);
                    wl_surface_commit(pWindow->GetSurface());

                    wl_display_flush(pWindow->GetDisplay());
                }
            }

            if (width > 0 && height > 0) {
                pWindow->SetWaylandConfigured(true);
            }
        }

        void xdg_toplevel_handle_close(void* pData, xdg_toplevel*) {
            SR_LOG("xdg_toplevel_handle_close() : window close requested by compositor");
            static_cast<WaylandWindow*>(pData)->SetValid(false);
            static_cast<WaylandWindow*>(pData)->SetClosed(true);
        }

        void xdg_wm_base_ping(void*, xdg_wm_base* base, uint32_t serial) {
            xdg_wm_base_pong(base, serial);
        }

        void decoration_handle_configure(void* data, struct zxdg_toplevel_decoration_v1 *decoration, uint32_t mode) {
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

        static const struct zxdg_toplevel_decoration_v1_listener decoration_listener = {.configure = decoration_handle_configure};

        bool CreateSurfaceBuffer(WaylandWindow::SurfaceBuffer* buffer, wl_shm* shm, struct wl_surface* surface, const char* name, uint32_t color) {
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
            memset(buffer->pixels, color, buffer->width * buffer->height * sizeof(uint32_t));// clear to color

            // create pool
            buffer->pool = wl_shm_create_pool(shm, buffer->fd, buffer->size);
            buffer->buffer = wl_shm_pool_create_buffer(buffer->pool, 0, buffer->width, buffer->height, buffer->stride, WL_SHM_FORMAT_XRGB8888);
            buffer->color = color;

            wl_surface_attach(surface, buffer->buffer, 0, 0);
            SR_LOG("WaylandWindow::CreateSurfaceBuffer() : created surface buffer ({}x{}, fd={}, size={})", buffer->width, buffer->height, buffer->fd, buffer->size);
            return true;
        }

        int ResizeSurfaceBuffer(WaylandWindow::SurfaceBuffer* buffer, wl_shm* shm, wl_surface* surface) {
            // pre-dispose old buffers
            munmap(buffer->pixels, buffer->size);
            wl_shm_pool_destroy(buffer->pool);
            struct wl_buffer* oldBuffer = buffer->buffer;// dispose after new buffer is created

            // create new buffer
            int result = CreateSurfaceBuffer(buffer, shm, surface, NULL, buffer->color);

            // post-dispose old buffer
            wl_buffer_destroy(oldBuffer);
            return result;
        }
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
    }

    void WaylandWindow::ThreadFunction() {
        SR_LOG("WaylandWindow::ThreadFunction() : thread started.");

        while (m_isValid && !m_isClosed) {
            //wl_display_dispatch_pending (display);

            if (wl_display_dispatch(m_display) < 0) {
                SR_ERROR("WaylandWindow::ThreadFunction() : wl_display_dispatch failed!");
                break;
            }
        }

        m_isClosed = true;
        m_isValid = false;

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

        /// create window
        m_surfaceBuffer.fd = -1;
        m_clientSurfaceBuffer.fd = -1;

        InternalSetWindowSize(320, 240);

        m_surface = wl_compositor_create_surface(m_compositor);
        m_xdgSurface = xdg_wm_base_get_xdg_surface(m_xdgWMBase, m_surface);
        m_xdgToplevel = xdg_surface_get_toplevel(m_xdgSurface);
        xdg_surface_add_listener(m_xdgSurface, &Details::xdg_surface_listener, this);
        xdg_toplevel_add_listener(m_xdgToplevel, &Details::xdg_toplevel_listener, this);
        xdg_wm_base_add_listener(m_xdgWMBase, &Details::xdg_wm_base_listener, this);
        xdg_toplevel_set_title(m_xdgToplevel, "WaylandClientWindow");
        xdg_toplevel_set_app_id(m_xdgToplevel, "WaylandClientWindow");
        xdg_toplevel_set_min_size(m_xdgToplevel, 10, 10);

        /// get server-side decorations
        if (!m_useClientDecorations && m_decorationManager) {
            m_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(m_decorationManager, m_xdgToplevel);
            zxdg_toplevel_decoration_v1_add_listener(m_decoration, &Details::decoration_listener, this);
        }

        // surface buffers
        uint32_t color = m_useClientDecorations ? Details::ToColor(127, 127, 127, 255) : Details::ToColor(255, 255, 255, 255);
        if (Details::CreateSurfaceBuffer(&m_surfaceBuffer, GetShm(), m_surface, "WaylandClientWindow_Decorations", color) != 1) {
            SR_ERROR("WaylandWindow::Initialize() : failed to create surface buffer!");
            return false;
        }

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

        if (!SR_HTYPES_NS::Thread::Factory::Instance().Create(m_thread, &WaylandWindow::ThreadFunction, this)) {
            SRHalt("WaylandWindow::Initialize() : failed to create window thread!");
            return false;
        }

        m_thread->SetName("Wayland window");

        if (!m_configured) {
            SR_LOG("WaylandWindow::Initialize() : waiting for Wayland configuration...");

            while (!m_configured && m_isValid && !m_isClosed) {
                SR_NOOP;
            }

            SR_LOG("WaylandWindow::Initialize() : Wayland configuration received.");
        }

        return true;
    }

    void WaylandWindow::PollEvents() {
        Super::PollEvents();
    }

    void WaylandWindow::Close() {
        SR_LOG("WaylandWindow::Close() : closing window...");

        Super::Close();

        if (m_thread && m_thread->Joinable()) {
            m_thread->Join();
            m_thread->Free();
        }

        SR_LOG("WaylandWindow::Close() : destroying Wayland resources...");

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
