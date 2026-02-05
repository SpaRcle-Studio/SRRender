//
// Created by monika on 2/4/26.
//

#ifndef SR_ENGINE_GRAPHICS_WAYLAND_WINDOW_H
#define SR_ENGINE_GRAPHICS_WAYLAND_WINDOW_H

#include <Graphics/Window/BasicWindowImpl.h>

#include <Utils/Math/Rect.h>
#include <Utils/Types/Thread.h>

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
        void SetDecorationMode(zxdg_toplevel_decoration_v1_mode mode) { m_currentDecorationMode = mode; }
        void SetWaylandConfigured(const bool configured) { m_configured = configured; }
        void SetWaitingForConfigure(const bool waiting) { m_waitingForConfigure = waiting; }

        SR_NODISCARD wl_cursor_theme* GetCursorTheme() const { return m_cursorTheme; }
        SR_NODISCARD wl_surface* GetCursorSurface() const { return m_cursorSurface; }
        SR_NODISCARD wl_display* GetDisplay() const { return m_display; }
        SR_NODISCARD wl_seat* GetSeat() const { return m_seat; }
        SR_NODISCARD wl_shm* GetShm() const { return m_shm; }
        SR_NODISCARD xdg_wm_base* GetXdgWMBase() const { return m_xdgWMBase; }
        SR_NODISCARD wl_compositor* GetCompositor() const { return m_compositor; }
        SR_NODISCARD wl_subcompositor* GetSubCompositor() const { return m_subCompositor; }
        SR_NODISCARD zxdg_decoration_manager_v1* GetDecorationManager() const { return m_decorationManager; }
        SR_NODISCARD zxdg_toplevel_decoration_v1* GetDecoration() const { return m_decoration; }
        SR_NODISCARD wl_surface* GetSurface() const { return m_surface; }
        SR_NODISCARD wl_surface* GetClientSurface() const { return m_clientSurface; }
        SR_NODISCARD bool IsUseClientDecorations() const { return m_useClientDecorations; }
        SR_NODISCARD bool IsMaximized() const { return m_isMaximized; }
        SR_NODISCARD int GetCompositeWidth() const { return m_compositeWidth; }
        SR_NODISCARD int GetCompositeHeight() const { return m_compositeHeight; }
        SR_NODISCARD SurfaceBuffer& GetSurfaceBuffer() { return m_surfaceBuffer; }
        SR_NODISCARD SurfaceBuffer& GetClientSurfaceBuffer() { return m_clientSurfaceBuffer; }

        void SetMaximized(bool isMaximized) { m_isMaximized = isMaximized; }

        bool Initialize(const std::string& name,
                        const SR_MATH_NS::IVector2& position,
                        const SR_MATH_NS::UVector2& size,
                        bool fullScreen, bool resizable) override;

        SR_NODISCARD WindowType GetType() const override { return WindowType::Wayland; }
        SR_NODISCARD void* GetHandle() const override { return m_surface; }

        uint32_t GetSurfaceWidth() const override { return static_cast<uint32_t>(m_surfaceBuffer.width); }
        uint32_t GetSurfaceHeight() const override { return static_cast<uint32_t>(m_surfaceBuffer.height); }
        uint32_t GetWidth() const override { return static_cast<uint32_t>(m_internalWidth); }
        uint32_t GetHeight() const override { return static_cast<uint32_t>(m_internalHeight); }

        void PollEvents() override;
        void Close() override;

        void InternalSetWindowSize(int width, int height);
        bool ResizeSurfaceBuffer(SurfaceBuffer* pBuffer, wl_surface* pSurface);

    private:
        bool DoWaylandPollEvents();
        void ThreadFunction();

    private:
        int m_internalWidth = 0;
        int m_internalHeight = 0;
        int m_compositeWidth = 0;
        int m_compositeHeight = 0;
        bool m_useClientDecorations = false;
        bool m_isMaximized = false;

        std::atomic<bool> m_waitingForConfigure = false;
        std::atomic<bool> m_configured = false;

        SR_HTYPES_NS::Thread::Ptr m_thread = nullptr;

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
        zxdg_toplevel_decoration_v1_mode m_currentDecorationMode = ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE;

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
