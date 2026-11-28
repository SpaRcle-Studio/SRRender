//
// Created by Monika on 07.12.2022.
//

#include <Graphics/Window/Win32Window.h>

#include <ImmediateGUI/GUI/ImmediateGUI.h>

#include <Utils/Platform/Platform.h>

#define SR_BORDERSIZE 5 * 2

namespace SR_GRAPH_NS::WinAPI {
    SR_MAYBE_UNUSED static int GetBorderHeight(HWND hWnd) {
        RECT rcClient, rcWind;
        GetClientRect(hWnd, &rcClient);
        GetWindowRect(hWnd, &rcWind);
        return ((rcWind.right - rcWind.left) - rcClient.right) / 2;
    }
}

namespace SR_GRAPH_NS {
    void Win32Window::SetFullScreen(bool value) {
        if (m_hWnd) {
            PostMessage(m_hWnd, WM_APP + 0x135, value ? 1 : 0, 0);
        }
    }

    LRESULT Win32Window::ReadWmdProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)  {
        SR_TRACY_ZONE;

        switch (msg) {
            case WM_APP + 0x135: {
                const bool fullscreen = wParam != 0;
                if (fullscreen == m_fullscreen.load()) {
                    return 0;
                }
                if (fullscreen) {
                    MONITORINFO monitor = { sizeof(MONITORINFO) };
                    if (!GetWindowPlacement(hwnd, &m_windowedPlacement)
                        || !GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor)) {
                        SR_ERROR("Win32Window::ReadWmdProcedure() : unable to query fullscreen monitor.");
                        return 0;
                    }
                    m_windowedStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
                    m_windowedHeaderEnabled = m_headerEnabled;
                    /// Флаг выставляется до снятия заголовка: другие потоки не должны видеть окно без заголовка как оконное.
                    m_fullscreen = true;
                    m_headerEnabled = false;
                    SetWindowLongPtr(hwnd, GWL_STYLE, m_windowedStyle & ~WS_OVERLAPPEDWINDOW);
                    SetWindowPos(hwnd, HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top,
                        monitor.rcMonitor.right - monitor.rcMonitor.left, monitor.rcMonitor.bottom - monitor.rcMonitor.top,
                        SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                }
                else {
                    m_headerEnabled = m_windowedHeaderEnabled;
                    SetWindowLongPtr(hwnd, GWL_STYLE, m_windowedStyle);
                    SetWindowPlacement(hwnd, &m_windowedPlacement);
                    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                }
                m_fullscreen = fullscreen;
                return 0;
            }
            case WM_CREATE: {
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_NCHITTEST: {
                if (m_fullscreen.load()) {
                    return HTCLIENT;
                }
                auto point = POINT { LOWORD(lParam), HIWORD(lParam) };

                /// Convert screen coordinates into client
                if (!::ScreenToClient(m_hWnd, &point)) {
                    return 0;
                }

                RECT rect;
                if(!GetClientRect(hwnd, &rect)) {
                    return 0;
                }

                rect.top -= 8; /// делаем проверку выше на 8 пикселей для удобства

                int32_t width = rect.right - rect.left;
                int32_t height = rect.bottom - rect.top;

                /// Decide what result message should have
                if ((std::abs(rect.left + width - point.x) < SR_BORDERSIZE) && (std::abs(rect.top + height - point.y) < SR_BORDERSIZE))
                    return HTBOTTOMRIGHT;
                else if ((std::abs(rect.left - point.x) < SR_BORDERSIZE) && (std::abs(rect.top + height - point.y) < SR_BORDERSIZE))
                    return HTBOTTOMLEFT;
                else if ((std::abs(rect.left + width - point.x) < SR_BORDERSIZE) && (std::abs(rect.top - point.y) < SR_BORDERSIZE))
                    return HTTOPRIGHT;
                else if ((std::abs(rect.left - point.x) < SR_BORDERSIZE) && (std::abs(rect.top - point.y) < SR_BORDERSIZE))
                    return HTTOPLEFT;
                else if (std::abs(rect.left - point.x) < SR_BORDERSIZE)
                    return HTLEFT;
                else if (std::abs(rect.top - point.y) < SR_BORDERSIZE)
                    return HTTOP;
                else if (std::abs(rect.left + width - point.x) < SR_BORDERSIZE)
                    return HTRIGHT;
                else if (std::abs(rect.top + height - point.y) < SR_BORDERSIZE)
                    return HTBOTTOM;
                else {
                    return DefWindowProc(hwnd, msg, wParam, lParam);
                }
            }
            case WM_CLOSE: {
                SR_SYSTEM_LOG("Win32Window::CallBack() : WM_CLOSE event has been received!");

                if (m_closeCallback) {
                    m_closeCallback(this);
                    return 0;
                }

                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_CHANGEUISTATE: {
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_DPICHANGED: {
                SR_LOG("Win32Window::ReadWmdProcedure() : WM_DPICHANGED. DPI has been changed!");
                m_screenDPI.reset();
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_WINDOWPOSCHANGING: {
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_WINDOWPOSCHANGED: {
                DWORD styles = GetWindowLongPtr(hwnd, GWL_STYLE);
                m_maximize = styles & WS_MAXIMIZE;
                m_collapsed = styles & WS_MINIMIZE;
                SR_DEBUG_LOG("Win32Window::ReadWmdProcedure() : WM_WINDOWPOSCHANGED. Window position has been changed! Maximize: {}, Collapsed: {}", m_maximize, m_collapsed);
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_SETFOCUS: {
                if (m_focusCallback) {
                    m_focusCallback(this, true);
                }
                m_isFocused = true;
                SR_DEBUG_LOG("Win32Window::ReadWmdProcedure() : WM_SETFOCUS. Window is focused now.");
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_KILLFOCUS: {
                if (m_focusCallback) {
                    m_focusCallback(this, false);
                }
                m_isFocused = false;
                SR_DEBUG_LOG("Win32Window::ReadWmdProcedure() : WM_KILLFOCUS. Window is not focused now.");
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_MOVE: {
                m_position.x = (int)(short) LOWORD(lParam);
                m_position.y = (int)(short) HIWORD(lParam);
                SR_LOG("Win32Window::ReadWmdProcedure() : WM_MOVE. Window position has been changed! New position: ({}, {})", m_position.x, m_position.y);
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_SIZE: {
                switch (wParam) {
                    case SIZE_MINIMIZED: m_state = WindowState::Collapsed; break;
                    case SIZE_MAXIMIZED: m_state = WindowState::Maximized; break;
                    case SIZE_RESTORED:  m_state = WindowState::Default;   break;
                    default:
                        break;
                }

                if (wParam != SIZE_MINIMIZED) {
                    if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) {
                        m_surfaceSize = SR_MATH_NS::UVector2(LOWORD(lParam), HIWORD(lParam));

                        if (m_surfaceSize.x != 0 && m_styleState != WinAPI::StyleState::Changing) {
                            m_size.x = m_surfaceSize.x;
                        }

                        if (m_surfaceSize.y != 0 && m_styleState != WinAPI::StyleState::Changing) {
                            m_size.y = m_headerEnabled ? m_surfaceSize.y + 30 : m_surfaceSize.y;
                        }

                        m_collapsed = m_surfaceSize.HasZero();

                        if (m_resizeCallback) {
                            m_resizeCallback(this, GetSurfaceWidth(), GetSurfaceHeight());
                        }
                    }
                }
                else {
                    m_surfaceSize = SR_MATH_NS::UVector2();
                    m_size = SR_MATH_NS::UVector2();

                    m_collapsed = m_surfaceSize.HasZero();

                    if (m_resizeCallback) {
                        m_resizeCallback(this, GetSurfaceWidth(), GetSurfaceHeight());
                    }
                }

                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_MOUSEWHEEL: {
                auto&& wheel = GET_WHEEL_DELTA_WPARAM(wParam);

                if (m_scrollCallback) {
                    m_scrollCallback(this, 0.0, (double) wheel / 22.0 / 5.0);
                }

                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_STYLECHANGING:
            case WM_STYLECHANGED: {
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_SYSCOMMAND: {
                /// Alt не должен переводить окно в режим системного меню: оно забирает фокус клавиатуры (Alt+Enter и т.п.).
                if ((wParam & 0xFFF0) == SC_KEYMENU) {
                    return 0;
                }

                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_SYSCHAR: {
                /// Alt+Enter is application input: without this the window has no menu for the mnemonic and Windows beeps.
                if (wParam == VK_RETURN) {
                    return 0;
                }

                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            case WM_DESTROY: {
                PostQuitMessage(0);
                m_isValid = false;
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            default:
                return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    SR_MATH_NS::IVector2 Win32Window::GetPosition() const {
        /*HWND hWndParent = GetParent(m_hWnd);
        POINT p = {0};

        int32_t offsetY = m_headerEnabled ? (31) : 0;
        int32_t offsetX = m_headerEnabled ? ((16 / 2)) : 0;

        MapWindowPoints(m_hWnd, hWndParent, &p, 1);

        return SR_MATH_NS::IVector2(p.x, p.y + offsetY);*/

        return m_position;
    }

    void Win32Window::Move(int32_t x, int32_t y) {
        /// В полноэкранном режиме окно занимает монитор; оконная позиция восстановится из m_windowedPlacement.
        if (m_maximize || m_fullscreen.load()) {
            return;
        }

        RECT rect;
        GetWindowRect(m_hWnd, &rect);

        int32_t offsetX = m_headerEnabled ? 8 : 0;

        MoveWindow(m_hWnd, x - offsetX, y,
                   (rect.right  - rect.left),
                   (rect.bottom - rect.top), FALSE
       );
    }

    void Win32Window::Maximize() {
        if (m_fullscreen.load()) {
            m_windowedPlacement.showCmd = SW_SHOWMAXIMIZED;
            return;
        }

        ShowWindow(m_hWnd, SW_MAXIMIZE);
    }

    void Win32Window::Restore() {
        if (m_fullscreen.load()) {
            m_windowedPlacement.showCmd = SW_SHOWNORMAL;
            return;
        }

        ShowWindow(m_hWnd, SW_RESTORE);
    }

    void Win32Window::Collapse() {
        ShowWindow(m_hWnd, SW_MINIMIZE);
    }

    void Win32Window::Expand() {
        ShowWindow(m_hWnd, SW_SHOW);
    }

    void Win32Window::SetHeaderEnabled(bool enable) {
        if (m_headerEnabled == enable) {
            return;
        }

        if (m_fullscreen.load()) {
            m_windowedHeaderEnabled = enable;
            return;
        }

        if (!enable && !SR_UTILS_NS::Features::Instance().Enabled("BorderlessWindow")) {
            return;
        }

        m_styleState = WinAPI::StyleState::Changing;

        m_headerEnabled = enable;

        /// костыль, чтобы окно всегда получало событие WM_SIZE, не зависимо от того,
        /// изменился ли размер, иначе будет deadlock
        /// SendMessage(m_hWnd, WM_SIZE, 0, 0);

        if (m_headerEnabled) {
            m_dwStyle |= WS_CAPTION;
            m_dwStyle |= WS_SIZEBOX;
            m_dwStyle |= WS_SYSMENU;
        }
        else {
            m_dwStyle &= ~WS_CAPTION;
            m_dwStyle &= ~WS_SIZEBOX;
            m_dwStyle &= ~WS_SYSMENU;
        }

        SetWindowLong(m_hWnd, GWL_STYLE, m_dwStyle);
        UpdateWindow(m_hWnd);

        m_styleState = WinAPI::StyleState::Changed;

        Resize(m_size.x, m_size.y);
        ///Centralize(); - Централизируем по надобности в Engine::CreateMainWindow().

        ShowWindow(m_hWnd, SW_SHOW);
    }

    void Win32Window::Centralize() {
        auto&& screenSize = GetScreenResolution();

        uint32_t headerSize = m_headerEnabled ? 30 : 0;

        auto posX = (screenSize.x / 2 - m_size.x / 2);
        auto posY = (screenSize.y / 2 - m_size.y / 2);

        Move(posX, posY);
    }

    /// ЭТО ВСЕ НЕ РАБОТАЕТ, ОКНУ ПОХУЙ ОНО РАСТЯГИВАЕТСЯ ТАК КАК ХОЧЕТ, ВИНАПИ ДЕРЬМО
    void Win32Window::Resize(uint32_t w, uint32_t h) {
        SR_TRACY_ZONE;

        SR_LOG("Win32Window::Resize() : set new sizes {}x{}", w, h);

        /// Иначе восстановление размера окна при старте выводит его из полноэкранного режима, а флаг остаётся true.
        if (m_fullscreen.load()) {
            return;
        }

        RECT newRect = RECT{
                .left = 0L,
                .top = 0,
                .right = static_cast<LONG>(m_headerEnabled ? w + 8 : w),
                .bottom = static_cast<LONG>(m_headerEnabled ? h + 1 : h)
        };
        
        AdjustWindowRectEx(&newRect, m_dwStyle, FALSE, m_dwExStyle);

        if (!SetWindowPos(m_hWnd, NULL, newRect.left, newRect.top, newRect.right, newRect.bottom, SWP_NOMOVE)) {
            SRAssert(false);
        }

        UpdateWindow(m_hWnd);
    }

    LRESULT Win32Window::WndProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        SR_TRACY_ZONE;

        if (SR_IMMEDIATE_GUI_NS::WindowsWndProcHandler(hWnd, message, wParam, lParam)) {
            return true;
        }

        if (auto&& pWindow = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA))) {
            return pWindow->ReadWmdProcedure(hWnd, message, wParam, lParam);
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    SR_MATH_NS::IVector2 Win32Window::GetScreenResolution() const {
        SR_TRACY_ZONE;
        auto const hMonitor { ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONULL) };

        MONITORINFOEXW monInfo;
        monInfo.cbSize = sizeof(monInfo);
        if (!GetMonitorInfoW(hMonitor, &monInfo)) {
            return SR_MATH_NS::IVector2(); /// Error
        }

        auto width = std::abs(monInfo.rcMonitor.left - monInfo.rcMonitor.right);
        auto height = std::abs(monInfo.rcMonitor.top  - monInfo.rcMonitor.bottom);

        return SR_MATH_NS::IVector2(width, height);
    }

    void Win32Window::SetIcon(const std::string &path) {
        SR_TRACY_ZONE;
        SR_LOG("Win32Window::SetIcon() : set icon... \n\tPath: " + path);

        HICON hWindowIcon    = NULL;
        HICON hWindowIconBig = NULL;

        hWindowIcon    = (HICON)LoadImage(NULL, path.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
        hWindowIconBig = (HICON)LoadImage(NULL, path.c_str(), IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
        SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hWindowIcon);
        SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hWindowIconBig);
    }

    bool Win32Window::Initialize(const std::string &name,
            const SR_MATH_NS::IVector2& position,
            const SR_MATH_NS::UVector2& size,
            bool fullscreen, bool resizable
    ) {
        SR_TRACY_ZONE;
        SR_LOG("Win32Window::Initialize() : create WinAPI window...");

        m_hInst = GetModuleHandleA(nullptr);

        m_size = size;

        std::string className = "SREngineWinClass";

        WNDCLASSEX wndClass;

        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = CS_HREDRAW | CS_VREDRAW;
        wndClass.lpfnWndProc = WndProcedure;
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra = 0;
        wndClass.hInstance = m_hInst;
        /// Exported games store their icon group with id 1 in the executable resources.
        HICON hExecutableIcon = LoadIcon(m_hInst, MAKEINTRESOURCE(1));

        wndClass.hIcon = hExecutableIcon ? hExecutableIcon : LoadIcon(NULL, IDI_APPLICATION);
        wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wndClass.lpszMenuName = NULL;
        wndClass.lpszClassName = className.c_str();
        wndClass.hIconSm = hExecutableIcon ? hExecutableIcon : LoadIcon(NULL, IDI_WINLOGO);

        auto&& registerResult = RegisterClassEx(&wndClass);

        if (!registerResult && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) { /// NOLINT
            SR_ERROR("Win32Window::Initialize() : failed to register class!\n\tError code: " + SR_UTILS_NS::ToString(GetLastError()));
            return false;
        }

        m_dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        m_dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

        {
            m_hWnd = CreateWindowEx(
                0, className.c_str(),
                name.c_str(),
                m_dwStyle,
                position.x - 8, position.y,
                m_size.x,
                m_size.y,
                nullptr,
                nullptr,
                m_hInst,
                nullptr
            );
        }

        if (!m_hWnd) {
            SR_ERROR("Win32Window::Initialize() : failed to create WinAPI window!");
            return false;
        }

        SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);

        ShowWindow(m_hWnd, SW_SHOW);
        SetForegroundWindow(m_hWnd);
        SetFocus(m_hWnd);

        Resize(m_size.x, m_size.y);

        m_hDC = GetDC(m_hWnd);

        m_isValid = true;
        m_isInitialized = true;

        return true;
    }

    void Win32Window::SwapBuffers() const {
        SR_TRACY_ZONE;
        ::SwapBuffers(m_hDC);
    }

    void Win32Window::PollEvents() {
        SR_TRACY_ZONE;

        MSG msg = {};

        if (SR_IMMEDIATE_GUI_NS::GetCurrentContext()) {
            SR_IMMEDIATE_GUI_NS::GetViewports(m_viewports);
            for (auto&& pViewport : m_viewports) {
                auto&& pPlatformHandle = SR_IMMEDIATE_GUI_NS::GetViewportPlatformHandle(pViewport);
                while (::PeekMessage(&msg, (HWND)pPlatformHandle, 0, 0, PM_NOREMOVE)) {
                    SR_TRACY_ZONE_N("Win32Window::PollEvents() : viewport message loop");
                    if (!::GetMessage(&msg, (HWND)pPlatformHandle, 0, 0))
                        break;
                    ::TranslateMessage(&msg);
                    ::DispatchMessage(&msg);
                }
            }
        }

        while (::PeekMessage(&msg, m_hWnd, 0, 0, PM_NOREMOVE)) {
            SR_TRACY_ZONE_N("Win32Window::PollEvents() : main message loop");
            if (!::GetMessage(&msg, m_hWnd, 0, 0))
                break;
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            SR_TRACY_ZONE_N("Win32Window::PollEvents() : global message loop");
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void* Win32Window::GetHandle() const {
        return reinterpret_cast<void*>(m_hWnd);
    }

    void Win32Window::Close() {
        SR_TRACY_ZONE;

        if (m_hWnd && IsValid()) {
            DestroyWindow(m_hWnd);
            ::UnregisterClass(nullptr, (HINSTANCE)SR_PLATFORM_NS::GetInstance());
            m_hWnd = nullptr;
        }

        BasicWindowImpl::Close();
    }

    SR_MATH_NS::IVector2 Win32Window::ScreenToClient(const SR_MATH_NS::IVector2& pos) const {
        SR_TRACY_ZONE;
        POINT pt = { static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y) };
        ::ScreenToClient(m_hWnd, &pt);
        return SR_MATH_NS::IVector2(pt.x, pt.y);
    }

    SR_MATH_NS::IVector2 Win32Window::ClientToScreen(const SR_MATH_NS::IVector2& pos) const {
        SR_TRACY_ZONE;
        POINT pt = { static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y) };
        ::ClientToScreen(m_hWnd, &pt);
        return SR_MATH_NS::IVector2(pt.x, pt.y);
    }

    bool Win32Window::IsVisible() const {
        SR_TRACY_ZONE;
        return ::IsWindowVisible(m_hWnd);
    }

    float_t Win32Window::GetScreenDPI() const {
        if (!m_screenDPI) {
            UINT dpi = GetDpiForWindow(m_hWnd);
            m_screenDPI = static_cast<float_t>(dpi);
        }
        return m_screenDPI.value_or(0.f);
    }
}
