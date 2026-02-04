//
// Created by monika on 2/4/26.
//

#ifndef SR_ENGINE_GRAPHICS_WAYLAND_WINDOW_H
#define SR_ENGINE_GRAPHICS_WAYLAND_WINDOW_H

#include <Graphics/Window/BasicWindowImpl.h>

#include <Utils/Math/Rect.h>

#include <wayland-util.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <linux/input.h>

extern "C" {
    #include <xdg-shell-client-protocol.h>
    #include <xdg-decoration-unstable-v1.h>
}

namespace SR_GRAPH_NS {
    class WaylandWindow : public BasicWindowImpl {
        using Super = BasicWindowImpl;
    public:
        struct SurfaceBuffer {
            int width = 0, height = 0;
            uint32_t color = 0;
            char* name = nullptr;
            int fd = -1;
            int stride = 0, size = 0;
            struct wl_shm_pool* pool = nullptr;
            struct wl_buffer* buffer = nullptr;
            uint32_t* pixels = nullptr;
        };
    public:
        explicit WaylandWindow()
            : Super()
        { }

    public:
        void SetCompositor(struct wl_compositor* pCompositor) { m_compositor = pCompositor; }
        void SetSubCompositor(struct wl_subcompositor* pSubCompositor) { m_subCompositor = pSubCompositor; }
        void SetDecorationManager(zxdg_decoration_manager_v1* pDecorationManager) { m_decorationManager = pDecorationManager; }
        void SetDecoration(zxdg_toplevel_decoration_v1* pDecoration) { m_decoration = pDecoration; }
        void SetDisplay(wl_display* pDisplay) { m_display = pDisplay; }
        void SetSeat(wl_seat* pSeat) { m_seat = pSeat; }
        void SetShm(wl_shm* pShm) { m_shm = pShm; }
        void SetCursorSurface(wl_surface* pCursorSurface) { m_cursorSurface = pCursorSurface; }
        void SetCursorTheme(wl_cursor_theme* pCursorTheme) { m_cursorTheme = pCursorTheme; }
        void SetXdgWMBase(xdg_wm_base* pXdgWMBase) { m_xdgWMBase = pXdgWMBase; }

        SR_NODISCARD wl_cursor_theme* GetCursorTheme() const { return m_cursorTheme; }
        SR_NODISCARD wl_surface* GetCursorSurface() const { return m_cursorSurface; }
        SR_NODISCARD wl_display* GetDisplay() const { return m_display; }
        SR_NODISCARD wl_seat* GetSeat() const { return m_seat; }
        SR_NODISCARD wl_shm* GetShm() const { return m_shm; }
        SR_NODISCARD xdg_wm_base* GetXdgWMBase() const { return m_xdgWMBase; }
        SR_NODISCARD wl_compositor* GetCompositor() const { return m_compositor; }

        bool Initialize(const std::string& name,
                        const SR_MATH_NS::IVector2& position,
                        const SR_MATH_NS::UVector2& size,
                        bool fullScreen, bool resizable) override;

        SR_NODISCARD WindowType GetType() const override { return WindowType::Wayland; }
        SR_NODISCARD void* GetHandle() const override { return nullptr; }

        void PollEvents() override;

        void InternalSetWindowSize(int width, int height);

    private:
        int m_internalWidth = 0;
        int m_internalHeight = 0;
        int m_compositeWidth = 0;
        int m_compositeHeight = 0;
        bool m_useClientDecorations = false;

        SR_MATH_NS::IRect m_clientRectDragTopBar;

        SR_MATH_NS::IRect m_clientRectResizeLeftBar;
        SR_MATH_NS::IRect m_clientRectResizeRightBar;
        SR_MATH_NS::IRect m_clientRectResizeBottomBar;
        SR_MATH_NS::IRect m_clientRectResizeTopBar;

        SR_MATH_NS::IRect m_clientRectResizeTopLeft;
        SR_MATH_NS::IRect m_clientRectResizeTopRight;
        SR_MATH_NS::IRect m_clientRectResizeBottomLeft;
        SR_MATH_NS::IRect m_clientRectResizeBottomRight;

        SR_MATH_NS::IRect m_clientRectButtonClose;
        SR_MATH_NS::IRect m_clientRectButtonMax;
        SR_MATH_NS::IRect m_clientRectButtonMin;

        SurfaceBuffer m_surfaceBuffer;
        SurfaceBuffer m_clientSurfaceBuffer;

        wl_surface* m_surface = nullptr;
        wl_surface* m_clientSurface = nullptr;
        wl_subsurface* m_clientSubSurface = nullptr;
        wl_display* m_display = nullptr;
        wl_compositor* m_compositor = nullptr;
        wl_subcompositor* m_subCompositor = nullptr;
        wl_seat* m_seat = nullptr;
        wl_shm* m_shm = nullptr;
        wl_surface* m_cursorSurface = nullptr;
        wl_cursor_theme* m_cursorTheme = nullptr;

        xdg_surface* m_xdgSurface = nullptr;
        xdg_toplevel* m_xdgToplevel = nullptr;
        xdg_wm_base* m_xdgWMBase = nullptr;

        zxdg_decoration_manager_v1* m_decorationManager = nullptr;
        zxdg_toplevel_decoration_v1* m_decoration = nullptr;

    };
}

#endif //SR_ENGINE_GRAPHICS_WAYLAND_WINDOW_H
