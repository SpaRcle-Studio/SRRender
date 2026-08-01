//
// Created by monika on 2/4/26.
//

#ifndef SR_ENGINE_GRAPHICS_WAYLAND_WINDOW_H
#define SR_ENGINE_GRAPHICS_WAYLAND_WINDOW_H

#ifdef SR_COMMON_USE_NATIVE_WAYLAND
    #include <Graphics/Window/BasicWindowImpl.h>

    #include <Utils/Math/Rect.h>
    #include <Utils/Types/Thread.h>

    #include <linux/input.h>
    #include <wayland-client-core.h>
    #include <wayland-client-protocol.h>
    #include <wayland-client.h>
    #include <wayland-cursor.h>
    #include <wayland-util.h>
    #include <xkbcommon/xkbcommon.h>

extern "C" {
    #include <fractional-scale-v1-client-protocol.h>
    #include <pointer-constraints-unstable-v1.h>
    #include <relative-pointer-unstable-v1.h>
    #include <xdg-decoration-unstable-v1.h>
    #include <xdg-shell-client-protocol.h>
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
            int poolCapacity = 0;
            struct wl_shm_pool* pool = nullptr;
            struct wl_buffer* buffer = nullptr;
            uint32_t* pixels = nullptr;
        };
        struct Output {
            uint32_t name = -1;          // id, пришедший из registry
            wl_output* output = nullptr; // сам объект wl_output
            int scale = 0.f;             // текущий scale (wl_output.scale)
            int width = -1;              // физическая ширина монитора
            int height = -1;             // физическая высота
        };

    public:
        explicit WaylandWindow()
            : Super() {}

    public:
        bool Initialize(
            const std::string& name, const SR_MATH_NS::IVector2& position, const SR_MATH_NS::UVector2& size,
            bool fullScreen, bool resizabel
        ) override;

        SR_NODISCARD WindowType GetType() const override { return WindowType::Wayland; }
        SR_NODISCARD WindowState GetState() const override;
        SR_NODISCARD void* GetHandle() const override { return m_surface; }

        uint32_t GetSurfaceWidth() const override { return static_cast<uint32_t>(m_surfaceBuffer.width); }
        uint32_t GetSurfaceHeight() const override { return static_cast<uint32_t>(m_surfaceBuffer.height); }
        uint32_t GetWidth() const override { return static_cast<uint32_t>(m_internalWidth); }
        uint32_t GetHeight() const override { return static_cast<uint32_t>(m_internalHeight); }
        float GetScale() const { return m_fractionalScale ? m_fractionalScaleValue : static_cast<float>(m_scale); }


        // void Resize(uint32_t width, uint32_t height) { }
        // void Move(int32_t x, int32_t y) { }
        // void Centralize() { }
        // void Collapse() { }
        // void Expand() { }
        void Maximize() override;
        void Restore() override;
        void PollEvents() override;
        // void SwapBuffers() const { };
        // void SetIcon(const std::string& path) { }
        // void SetHeaderEnabled(bool enable) { }
        bool IsFullScreen() override {return m_isFullScreen;}
        void SetFullScreen(bool value) override;
        void Close() override;

    public:
        /// ========= INTERNAL FUNCTIONS =========

        SR_NODISCARD Output& FindOutput(wl_output* pOutput);

        void SetPendingSize(const SR_MATH_NS::IVector2& size) { m_pendingSize = size; }
        void SetMaximized(bool isMaximized) { m_isMaximized = isMaximized; }
        void SetFullscreenState(bool value) { m_isFullScreen = value; }

        
        void SetCompositor(struct wl_compositor* pCompositor) { m_compositor = pCompositor; }
        void SetSubCompositor(struct wl_subcompositor* pSubCompositor) { m_subCompositor = pSubCompositor; }
        void SetDecorationManager(zxdg_decoration_manager_v1* pDecorationManager) {
            m_decorationManager = pDecorationManager;
        }
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
        void SetFractionalScaleManager(wp_fractional_scale_manager_v1* pFractionalScaleManager) {
            m_fractionalScaleManager = pFractionalScaleManager;
        }
        void SetCursorPointer(wl_pointer* pPointer) { m_pointer = pPointer; }
        void SetKeyboard(wl_keyboard* pKeyboard) { m_keyboard = pKeyboard; }
        void SetXkbKeymap(xkb_keymap* pKeymap) { m_pXkbKeymap = pKeymap; }
        void SetXkbState(xkb_state* pState) { m_pXkbState = pState; }
        void SetPointerConstraints(zwp_pointer_constraints_v1* pConstraints) { m_pointerConstraints = pConstraints; }
        void SetRelativePointerManager(zwp_relative_pointer_manager_v1* pManager) {
            m_relativePointerManager = pManager;
        }
        void SetPointerEnterSerial(uint32_t serial) { m_pointerEnterSerial = serial; }

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
        SR_NODISCARD bool HasFractionalScale() const { return m_fractionalScale != nullptr; }
        SR_NODISCARD xkb_context* GetXkbContext() const { return m_xkbContext; }
        SR_NODISCARD xkb_keymap* GetXkbKeymap() const { return m_pXkbKeymap; }
        SR_NODISCARD xkb_state* GetXkbState() const { return m_pXkbState; }
        SR_NODISCARD wl_pointer* GetPointer() const { return m_pointer; }
        SR_NODISCARD zwp_pointer_constraints_v1* GetPointerConstraints() const { return m_pointerConstraints; }
        SR_NODISCARD zwp_relative_pointer_manager_v1* GetRelativePointerManager() const {
            return m_relativePointerManager;
        }
        SR_NODISCARD uint32_t GetPointerEnterSerial() const { return m_pointerEnterSerial; }
        SR_NODISCARD bool IsCursorLocked() const { return m_cursorLocked; }

        SR_MATH_NS::IVector2 GetMaxOutputResolution() const;

        void InternalSetWindowSize(int width, int height);
        void SetFractionalScale(const float scale) { m_fractionalScaleValue = scale; }
        bool ResizeSurfaceBuffer(SurfaceBuffer* pBuffer, wl_surface* pSurface);
        void AddOutput(Output output) { m_outputs.emplace_back(output); }
        void AddEnteredOutput(wl_output* pOutput) { m_enteredOutputs.insert(pOutput); }
        void RemoveEnteredOutput(wl_output* pOutput) { m_enteredOutputs.erase(pOutput); }
        void RecalculateScale();

    private:
        bool DoWaylandPollEvents();
        void ThreadFunction();
        void LockPointer();
        void UnlockPointer();

    private:
        std::optional<SR_MATH_NS::IVector2> m_pendingSize;
        SR_MATH_NS::IVector2 m_lastSize;

        int m_internalWidth = 0;
        int m_internalHeight = 0;
        int m_compositeWidth = 0;
        int m_compositeHeight = 0;
        bool m_useClientDecorations = false;
        bool m_isMaximized = false;
        bool m_isFullScreen = false;

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

        wl_keyboard* m_keyboard = nullptr;
        wl_pointer* m_pointer = nullptr;
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
        wp_fractional_scale_manager_v1* m_fractionalScaleManager = nullptr;
        wp_fractional_scale_v1* m_fractionalScale = nullptr;

        xkb_context* m_xkbContext = nullptr;
        xkb_keymap* m_pXkbKeymap = nullptr; xkb_state* m_pXkbState = nullptr;

        SR_UTILS_NS::Vector<Output> m_outputs;
        SR_UTILS_NS::Set<wl_output*> m_enteredOutputs;
        int m_scale = 1;
        float m_fractionalScaleValue = 1.f;

        xdg_surface* m_xdgSurface = nullptr;
        xdg_toplevel* m_xdgToplevel = nullptr;
        xdg_wm_base* m_xdgWMBase = nullptr;

        zxdg_decoration_manager_v1* m_decorationManager = nullptr;
        zxdg_toplevel_decoration_v1* m_decoration = nullptr;

        zwp_pointer_constraints_v1* m_pointerConstraints = nullptr;
        zwp_relative_pointer_manager_v1* m_relativePointerManager = nullptr;
        zwp_locked_pointer_v1* m_lockedPointer = nullptr;
        zwp_relative_pointer_v1* m_relativePointer = nullptr;
        uint32_t m_pointerEnterSerial = 0;
        bool m_cursorLocked = false;
    };
} // namespace SR_GRAPH_NS

#endif

#endif // SR_ENGINE_GRAPHICS_WAYLAND_WINDOW_H
