//
// Created by Nikita on 18.11.2020.
//

#include <Utils/Math/Vector2.h>
#include <Utils/Types/Thread.h>
#include <Utils/Input/InputSystem.h>
#include <Utils/Platform/Platform.h>
#include <Utils/Resources/ResourceManager.h>

#include <Graphics/Window/Window.h>
#include <Graphics/Types/Camera.h>
#include <Graphics/Material/BaseMaterial.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/Types/Framebuffer.h>
#include <Graphics/GUI/Editor/Theme.h>
#include <Graphics/GUI/WidgetManager.h>
#include <Graphics/GUI/Widget.h>

namespace SR_GRAPH_NS {
    Window::Window()
        : Super(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    Window::~Window() {
        SR_SAFE_DELETE_PTR(m_windowImpl);
    }

    bool Window::Initialize(const std::string& name, const SR_MATH_NS::UVector2& size) {
        SR_INFO("Window::Initialize() : initializing the window...");

        m_windowImpl = BasicWindowImpl::CreatePlatformWindow(BasicWindowImpl::WindowType::Auto);
        if (!m_windowImpl) {
            SR_ERROR("Window::Initialize() : failed to create window implementation!");
            return false;
        }

        m_name = name;
        m_initialSize = size;

        return true;
    }

    SR_MATH_NS::UVector2 Window::GetSize() const {
        if (!m_windowImpl) {
            SR_ERROR("Window::GetSize() : window implementation is nullptr.");
            return { };
        }

        return SR_MATH_NS::UVector2(m_windowImpl->GetWidth(), m_windowImpl->GetHeight());
    }

    void Window::SetDrawCallback(const Window::DrawCallback& callback) {
        m_drawCallback = callback;
    }

    bool Window::IsWindowFocus() const {
        return m_windowImpl->IsFocused();
    }

    bool Window::IsWindowCollapsed() const {
        return m_windowImpl->IsCollapsed();
    }

    void Window::SetCloseCallback(const Window::CloseCallback &callback) {
        m_closeCallback = callback;
    }

    void Window::Close() {
        if (m_windowImpl) {
            m_windowImpl->Close();
        }
    }

    Window::WindowHandle Window::GetHandle() const {
        if (!m_windowImpl) {
            SR_ERROR("Window::GetHandle() : window implementation is nullptr.");
            return nullptr;
        }
        return m_windowImpl->GetHandle();
    }

    bool Window::IsValid() const {
        if (!m_windowImpl) {
            return false;
        }

        return m_windowImpl->IsValid();
    }

    void Window::SetResizeCallback(const Window::ResizeCallback &callback) {
        m_windowImpl->SetResizeCallback([callback](auto&& pWin, int32_t width, int32_t height) {
            callback(SR_MATH_NS::UVector2(static_cast<uint32_t>(width), static_cast<uint32_t>(height)));
        });
    }

    bool Window::IsFullScreen() const {
        return false;
    }

    void Window::SetFullScreen(bool value) {

    }

    void Window::SetFocusCallback(const Window::FocusCallback &callback) {
        m_windowImpl->SetFocusCallback([callback](auto&& pWin, auto&& focus) {
            callback(focus);
        });
    }

    void Window::SetScrollCallback(const Window::ScrollCallback &callback) {
        m_windowImpl->SetScrollCallback([callback](auto&& pWin, auto&& xOffset, auto&& yOffset) {
            callback(xOffset, yOffset);
        });
    }

    SR_MATH_NS::IVector2 Window::ScreenToClient(const SR_MATH_NS::IVector2& pos) const {
        return m_windowImpl->ScreenToClient(pos);
    }

    SR_MATH_NS::IVector2 Window::ClientToScreen(const SR_MATH_NS::IVector2& pos) const {
        return m_windowImpl->ClientToScreen(pos);
    }

    SR_MATH_NS::IVector2 Window::GetPosition() const {
        if (!m_windowImpl) {
            SR_ERROR("Window::GetPosition() : window implementation is nullptr.");
            return { };
        }

        return m_windowImpl->GetPosition();
    }

    bool Window::IsMaximized() const {
        if (!m_windowImpl) {
            SR_ERROR("Window::IsMaximized() : window implementation is nullptr.");
            return false;
        }

        if (m_windowImpl->GetState() == WindowState::Maximized) {
            return true;
        }

        SR_NOOP;

        return false;
    }

    bool Window::IsVisible() const {
        if (!m_windowImpl) {
            SR_ERROR("Window::IsVisible() : window implementation is nullptr.");
            return false;
        }

        if (!m_windowImpl->IsValid()) {
            return false;
        }

        return m_windowImpl->IsVisible();
    }

    void Window::PollEvents() {
        SR_TRACY_ZONE;
        if (!m_windowImpl) {
            SR_ERROR("Window::PollEvents() : window implementation is nullptr.");
            return;
        }
        m_windowImpl->PollEvents();
    }

    bool Window::Open() {
        SR_LOG("Window::Open() : opening the window...");

        if (!m_windowImpl) {
            SR_ERROR("Window::Open() : window implementation is nullptr.");
            return false;
        }

        if (m_windowImpl->IsValid()) {
            SR_ERROR("Window::Open() : window is already opened!");
            return false;
        }

        if (!m_windowImpl->Initialize(m_name, SR_MATH_NS::IVector2(), m_initialSize, false, true)) {
            SR_ERROR("Window::ThreadFunction() : failed to initialize window implementation!");
            return false;
        }

        m_windowImpl->SetIcon(SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat("Engine/Textures/icon.ico"));

        return true;
    }
}