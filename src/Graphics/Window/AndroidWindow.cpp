//
// Created by Monika on 08.12.2022.
//

#include <Graphics/Window/AndroidWindow.h>
#include <Utils/Platform/AndroidInput.h>
#include <Utils/Platform/AndroidLifecycle.h>

namespace SR_GRAPH_NS {
    AndroidWindow::~AndroidWindow() {
        SR_PLATFORM_NS::SetAndroidRendererAttached(false);
        SR_PLATFORM_NS::ReleaseAndroidWindow(m_nativeWindow);
    }

    bool AndroidWindow::Initialize(const std::string& name,
        const SR_MATH_NS::IVector2& position, const SR_MATH_NS::UVector2& size,
        bool fullScreen, bool resizable)
    {
        const auto state = SR_PLATFORM_NS::AcquireAndroidWindowSnapshot();
        m_nativeWindow = state.window;
        m_windowRevision = state.revision;
        m_resumed = state.resumed;
        m_isValid = !state.destroying;
        m_isInitialized = m_isValid.load();
        m_isFocused = state.focused && m_resumed;
        m_surfaceSize = m_size = size;
        return m_isValid && m_nativeWindow;
    }

    ANativeWindow* AndroidWindow::GetNativeWindow() const {
        return m_nativeWindow;
    }

    bool AndroidWindow::IsSurfaceAvailable() const {
        SR_TRACY_PLOT("Android native window", static_cast<int64_t>(m_nativeWindow != nullptr));
        SR_TRACY_PLOT("Android resumed", static_cast<int64_t>(m_resumed));
        SR_TRACY_PLOT("Android surface ready", static_cast<int64_t>(m_surfaceReady));
        SR_TRACY_PLOT("Android surface pending", static_cast<int64_t>(m_surfacePending));
        SR_TRACY_PLOT("Android window valid", static_cast<int64_t>(m_isValid.load()));
        SR_TRACY_PLOT("Android window width", static_cast<int64_t>(m_nativeWindow ? ANativeWindow_getWidth(m_nativeWindow) : 0));
        SR_TRACY_PLOT("Android window height", static_cast<int64_t>(m_nativeWindow ? ANativeWindow_getHeight(m_nativeWindow) : 0));
        return m_nativeWindow && m_resumed && m_surfaceReady && !m_surfacePending && m_isValid &&
            ANativeWindow_getWidth(m_nativeWindow) > 0 && ANativeWindow_getHeight(m_nativeWindow) > 0;
    }

    void AndroidWindow::SetSurfaceChangeCallback(SR_HTYPES_NS::Function<bool(ANativeWindow*)> callback) {
        m_surfaceChange = std::move(callback);
        SR_PLATFORM_NS::SetAndroidRendererAttached(static_cast<bool>(m_surfaceChange));
    }

    SR_MATH_NS::IVector2 AndroidWindow::ScreenToClient(const SR_MATH_NS::IVector2& position) const {
        return position;
    }

    SR_MATH_NS::IVector2 AndroidWindow::ClientToScreen(const SR_MATH_NS::IVector2& position) const {
        return position;
    }

    void AndroidWindow::PollEvents() {
        const auto state = SR_PLATFORM_NS::AcquireAndroidWindowSnapshot();
        if (state.revision != m_windowRevision) {
            auto previous = m_nativeWindow;
            m_nativeWindow = state.window;
            m_windowRevision = state.revision;
            m_surfacePending = m_nativeWindow &&
                (ANativeWindow_getWidth(m_nativeWindow) <= 0 || ANativeWindow_getHeight(m_nativeWindow) <= 0);
            m_surfaceReady = !m_surfaceChange || m_surfaceChange(m_surfacePending ? nullptr : m_nativeWindow);
            SR_PLATFORM_NS::ReleaseAndroidWindow(previous);
            if (m_surfaceReady) {
                SR_PLATFORM_NS::AcknowledgeAndroidWindow(state.revision);
            }
            if (!m_surfaceReady) {
                SR_ERROR("AndroidWindow::PollEvents() : failed to replace the native render surface.");
                Close();
                if (m_closeCallback) {
                    m_closeCallback(this);
                }
            }
        }
        else {
            SR_PLATFORM_NS::ReleaseAndroidWindow(state.window);
        }
        m_resumed = state.resumed;
        const bool focused = state.focused && IsSurfaceAvailable();
        if (m_isFocused != focused) {
            m_isFocused = focused;
            if (!focused) {
                SR_PLATFORM_NS::ResetAndroidInput();
            }
            if (m_focusCallback) {
                m_focusCallback(this, focused);
            }
        }
        if (m_nativeWindow) {
            const int32_t width = ANativeWindow_getWidth(m_nativeWindow);
            const int32_t height = ANativeWindow_getHeight(m_nativeWindow);
            if (width > 0 && height > 0) {
                if (m_surfacePending) {
                    m_surfacePending = false;
                    m_surfaceReady = !m_surfaceChange || m_surfaceChange(m_nativeWindow);
                    if (!m_surfaceReady) {
                        Close();
                        if (m_closeCallback) {
                            m_closeCallback(this);
                        }
                    }
                }
                const SR_MATH_NS::UVector2 size(width, height);
                if (size != m_surfaceSize) {
                    m_surfaceSize = m_size = size;
                    if (m_resizeCallback) {
                        m_resizeCallback(this, width, height);
                    }
                }
            }
        }
        SR_UTILS_NS::AndroidEvent event{};
        while (SR_UTILS_NS::AndroidEventQueue::Instance().PopEvent(event)) {
            if (focused) {
                SR_PLATFORM_NS::ApplyAndroidInputEvent(event);
            }
        }
        if (state.destroying && m_isValid) {
            SR_PLATFORM_NS::ResetAndroidInput();
            Close();
            if (m_closeCallback) {
                m_closeCallback(this);
            }
        }
    }
}
