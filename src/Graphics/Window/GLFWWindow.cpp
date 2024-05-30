//
// Created by innerviewer on 2024-05-06.
//

#include <Graphics/Window/GLFWWindow.h>

#include <GLFW/glfw3.h>

namespace SR_GRAPH_NS {
    bool GLFWWindow::Initialize(
        const std::string &name, const SR_MATH_NS::IVector2 &position,
        const SR_MATH_NS::UVector2 &size, bool fullScreen, bool resizable
    ) {
        if (!glfwInit())
        {
            SR_ERROR("GLFWWindow::Initialize() : failed to initialize GLFW.");
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

        GLFWwindow* pWindow = glfwCreateWindow(size.x, size.y, name.c_str(), nullptr, nullptr);
        if (!pWindow) {
            SR_ERROR("GLFWWindow::Initialize() : failed to create window.");
            return false;
        }


        int32_t x = -1;
        int32_t y = -1;
        glfwGetWindowSize(pWindow, &x, &y);

        if (x != -1 || y != -1) {
            m_size = { static_cast<uint32_t>(x), static_cast<uint32_t>(y) };
            m_surfaceSize = m_size;
        }

        m_window = pWindow;

        glfwSetWindowUserPointer(pWindow, this);

        glfwSetWindowSizeCallback(m_window, [](GLFWwindow* pWindow, int width, int height) {
            if (void* pHandle = glfwGetWindowUserPointer(pWindow)) {
                static_cast<GLFWWindow*>(pHandle)->OnSizeChangedCallback(pWindow, { width, height });
            }
        });

        glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* pWindow, int width, int height) {
            if (void* pHandle = glfwGetWindowUserPointer(pWindow)) {
                static_cast<GLFWWindow*>(pHandle)->OnFramebufferSizeChangedCallback(pWindow, { width, height });
            }
        });

        glfwSetWindowFocusCallback(m_window, [](GLFWwindow* pWindow, int focused) {
            if (void* pHandle = glfwGetWindowUserPointer(pWindow)) {
                static_cast<GLFWWindow*>(pHandle)->OnFocusChangedCallback(pWindow, focused);
            }
        });

        glfwSetWindowRefreshCallback(m_window, [](GLFWwindow* pWindow){
            if (void* pHandle = glfwGetWindowUserPointer(pWindow)) {
                static_cast<GLFWWindow*>(pHandle)->OnWindowRefreshCallback(pWindow);
            }
        });

        glfwSetScrollCallback(m_window, [](GLFWwindow* pWindow, double xoffset, double yoffset) {
            if (void* pHandle = glfwGetWindowUserPointer(pWindow)) {
                static_cast<GLFWWindow*>(pHandle)->OnScrollCallback(pWindow, xoffset, yoffset);
            }
        });

        glfwClipbo

        //glfwSetMouseButtonCallback()

        m_isValid = true;
        glfwRequestWindowAttention(m_window);

        return true;
    }

    void GLFWWindow::PollEvents() {
        glfwPollEvents();

        BasicWindowImpl::PollEvents();
    }

    void GLFWWindow::OnSizeChangedCallback(GLFWwindow* pWindow, SR_MATH_NS::IVector2 size) {
        if (pWindow != m_window) {
            /// This is just for debug. I would just like to know how glfw handles events and if this case is actually possible.
            SR_WARN("GLFWWindow::OnSizeChangedCallback() : window handle mismatch.");
            return;
        }

        if (size.x == m_surfaceSize.x && size.y == m_surfaceSize.y) {
            return;
        }

        if (size.x < 0 || size.y < 0) {
            return;
        }

        m_size = { static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.x) };
        m_surfaceSize = m_size;

        if (m_resizeCallback) {
            m_resizeCallback(this, size.x, size.y);
        }

        if (ImGui::GetCurrentContext()) {
            ImGui::GetIO().DisplaySize.x = size.x;
            ImGui::GetIO().DisplaySize.y = size.y;
        }
    }

    void GLFWWindow::OnFramebufferSizeChangedCallback(GLFWwindow* pWindow, SpaRcle::Utils::Math::IVector2 size) {
        if (pWindow != m_window) {
            /// This is just for debug. I would just like to know how glfw handles events and if this case is actually possible.
            SR_WARN("GLFWWindow::OnFramebufferSizeChangedCallback() : window handle mismatch.");
            return;
        }

        if (size.x == m_surfaceSize.x && size.y == m_surfaceSize.y) {
            return;
        }

        if (size.x < 0 || size.y < 0) {
            return;
        }

        /*m_size = { static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.x) };
        m_surfaceSize = m_size;

        if (m_resizeCallback) {
            m_resizeCallback(this, size.x, size.y);
        }*/

        if (ImGui::GetCurrentContext()) {
            ImGui::GetIO().DisplaySize.x = size.x;
            ImGui::GetIO().DisplaySize.y = size.y;
        }
    }

    void GLFWWindow::OnFocusChangedCallback(GLFWwindow* pWindow, bool isFocused) {
        if (pWindow != m_window) {
            /// This is just for debug. I would just like to know how glfw handles events and if this case is actually possible.
            SR_WARN("GLFWWindow::OnFocusChangedCallback() : window handle mismatch.");
            return;
        }

        m_isFocused = isFocused;

        if (m_focusCallback) {
            m_focusCallback(this, isFocused);
        }
    }

    void GLFWWindow::OnWindowRefreshCallback(GLFWwindow* pWindow) {
        SR_INFO("GLFWWindow::OnWindowRefreshCallback() : ------------------------------");

        if (ImGui::GetCurrentContext()) {
            ImGui::GetIO().DisplaySize.x = m_size.x;
            ImGui::GetIO().DisplaySize.y = m_size.y;
        }
    }

    void GLFWWindow::OnScrollCallback(GLFWwindow* pWindow, double xoffset, double yoffset) {
        if (pWindow != m_window) {
            /// This is just for debug. I would just like to know how glfw handles events and if this case is actually possible.
            SR_WARN("GLFWWindow::OnScrollCallback() : window handle mismatch.");
            return;
        }

        if (m_scrollCallback) {
            m_scrollCallback(this, xoffset, yoffset);
        }
    }

    void GLFWWindow::SetIcon(const std::string& path) {
        // TODO: Implement methods in SRCommon to load an image, otherwise we cannot set the icon.

        /*GLFWimage image;


        glfwSetWindowIcon(m_window, 1, &image);*/

        BasicWindowImpl::SetIcon(path);
    }
}
