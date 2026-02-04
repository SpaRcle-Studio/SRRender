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

        void registry_add_object(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
        void registry_remove_object(void *data, struct wl_registry *registry, uint32_t name);
        wl_registry_listener registry_listener = {&registry_add_object, &registry_remove_object};

        void registry_add_object(void* pData, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
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
                // TODO: wl_seat_add_listener(pSeat, &seat_listener, pData);
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

        void registry_remove_object(void *data, struct wl_registry *registry, uint32_t name) {
            // do nothing...
        }

        void seat_capabilities(void* pData, struct wl_seat* pSeat, uint32_t capabilities);
        struct wl_seat_listener seat_listener = {&seat_capabilities};

        void seat_capabilities(void* pData, struct wl_seat* pSeat, uint32_t capabilities) {
            auto&& pWindow = static_cast<WaylandWindow*>(pData);
            if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
                /// struct wl_pointer *pointer = wl_seat_get_pointer(pSeat);
                // TODO: wl_pointer_add_listener(pointer, &pointer_listener, pData);
                pWindow->SetCursorSurface(wl_compositor_create_surface(pWindow->GetCompositor()));
            }
        }

        bool CreateSurfaceBuffer(WaylandWindow::SurfaceBuffer* buffer, wl_shm* shm, struct wl_surface* surface, const char* name, uint32_t color) {
            // get buffer sizes
            int oldSize = buffer->size;
            buffer->stride = buffer->width * sizeof(uint32_t);
            buffer->size = buffer->height * buffer->stride;

            // alloc name if needed
            if (name != NULL)
            {
                if (buffer->name != NULL)
                    free(buffer->name);
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

        /// create window
        m_surfaceBuffer.fd = -1;
        m_clientSurfaceBuffer.fd = -1;

        InternalSetWindowSize(320, 240);

        m_surface = wl_compositor_create_surface(m_compositor);
        m_xdgSurface = xdg_wm_base_get_xdg_surface(m_xdgWMBase, m_surface);
        m_xdgToplevel = xdg_surface_get_toplevel(m_xdgSurface);
        // TODO: xdg_surface_add_listener(m_xdgSurface, &xdg_surface_listener, this);
        // TODO: xdg_toplevel_add_listener(m_xdgToplevel, &xdg_toplevel_listener, this);
        // TODO: xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, this);
        xdg_toplevel_set_title(m_xdgToplevel, "WaylandClientWindow");
        xdg_toplevel_set_app_id(m_xdgToplevel, "WaylandClientWindow");
        xdg_toplevel_set_min_size(m_xdgToplevel, 100, 100);

        /// get server-side decorations
        if (!m_useClientDecorations && m_decorationManager) {
            m_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(m_decorationManager, m_xdgToplevel);
            /// TODO: zxdg_toplevel_decoration_v1_add_listener(decoration, &decoration_listener, NULL);
        }

        // surface buffers
        uint32_t color = m_useClientDecorations ? Details::ToColor(127, 127, 127, 255) : Details::ToColor(255, 255, 255, 255);
        if (Details::CreateSurfaceBuffer(&m_surfaceBuffer, GetShm(), m_surface, "WaylandClientWindow_Decorations", color) != 1) {
            SR_ERROR("WaylandWindow::Initialize() : failed to create surface buffer!");
            return false;
        }

        if (m_useClientDecorations)
        {
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

        // event loop
        while (true)
        {
            //wl_display_dispatch_pending (display);
            if (wl_display_dispatch(m_display) < 0) break;
        }

        return false;
    }

    void WaylandWindow::PollEvents() {
        Super::PollEvents();
    }
}
