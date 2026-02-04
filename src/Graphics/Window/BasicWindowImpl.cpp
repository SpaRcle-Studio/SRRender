//
// Created by Monika on 07.12.2022.
//

#include <Graphics/Window/BasicWindowImpl.h>
#include <Utils/Debug.h>
#include <Utils/Common/CLIManager.h>

#include <Graphics/Window/HeadlessWindow.h>

#if defined(SR_WIN32)
    #include <Graphics/Window/Win32Window.h>
#elif defined(SR_ANDROID)
    #include <Graphics/Window/AndroidWindow.h>
#elif defined(SR_LINUX)
    //#include <Graphics/Window/X11Window.h>
    #include <Graphics/Window/GLFWWindow.h>
    #include <Graphics/Window/WaylandWindow.h>
#endif

#include <Enum/WindowProtocolType.hpp>

namespace SR_GRAPH_NS {
    BasicWindowImpl::~BasicWindowImpl() {
        SRAssert(!IsValid());
    }

    void BasicWindowImpl::SetResizeCallback(const ResizeCallback& callback) {
        m_resizeCallback = callback;
    }

    void BasicWindowImpl::SetMoveCallback(const MoveCallback& callback) {
        m_moveCallback = callback;
    }

    void BasicWindowImpl::SetScrollCallback(const ScrollCallback& callback) {
        m_scrollCallback = callback;
    }

    void BasicWindowImpl::SetFocusCallback(const FocusCallback& callback) {
        m_focusCallback = callback;
    }

    void BasicWindowImpl::SetCloseCallback(const CloseCallback& callback) {
        m_closeCallback = callback;
    }

    void BasicWindowImpl::SetDrawCallback(const BasicWindowImpl::DrawCallback &callback) {
        m_drawCallback = callback;
    }

    BasicWindowImpl* BasicWindowImpl::CreatePlatformWindow(BasicWindowImpl::WindowType type) {
        if (SR_UTILS_NS::CLIManager::Instance().IsHeadlessMode()) {
            SR_INFO("BasicWindowImpl::CreatePlatformWindow() : running in headless mode!");
            return new HeadlessWindow();
        }

    #if defined(SR_WIN32)
        switch (type) {
            case WindowType::Auto:
            case WindowType::Win32:
                return new Win32Window();
            default:
                break;
        }

        SR_ERROR("BasicWindowImpl::CreatePlatformWindow() : OS Windows not support \"" +
            SR_UTILS_NS::EnumReflector::ToStringAtom(type).ToStringRef() + "\" window!"
        );
    #elif defined(SR_ANDROID)
        switch (type) {
            case WindowType::Auto:
            case WindowType::Android:
                return new AndroidWindow();
            default:
                break;
        }

        SR_ERROR("BasicWindowImpl::CreatePlatformWindow() : OS Android not support \"" +
                 SR_UTILS_NS::EnumReflector::ToStringAtom(type).ToStringRef() + "\" window!"
        );
    #elif defined (SR_LINUX)
        switch (type) {
            case WindowType::Auto: {
                const auto protocolType = SR_PLATFORM_NS::GetWindowProtocolType();
                SR_LOG("BasicWindowImpl::CreatePlatformWindow() : detected window protocol type: {}", protocolType);
                switch (protocolType) {
                    case SR_PLATFORM_NS::WindowProtocolType::Wayland:
                        return new WaylandWindow();
                    default:
                        break;
                }
                SRHalt("Failed to determine window protocol type!");
                break;
            }
            case WindowType::Wayland:
                return new WaylandWindow();
        #ifdef SR_RENDER_GLFW
            case WindowType::GLFW:
                return new GLFWWindow();
        #endif
            case WindowType::X11:
                SRHalt("X11Window is not implemented yet!");
            default:
                SRHalt("Unsupported window type for Linux!");
                break;
        }

        SR_ERROR("BasicWindowImpl::CreatePlatformWindow() : implementation does not yet support \"" +
                 SR_UTILS_NS::EnumReflector::ToStringAtom(type).ToStringRef() + "\" window!"
        );
    #else
        SR_ERROR("BasicWindowImpl::CreatePlatformWindow() : unsupported OS!");
    #endif

        return nullptr;
    }

    void *BasicWindowImpl::GetHandle() const {
        SRHaltOnce("Not implemented!");
        return nullptr;
    }
}
