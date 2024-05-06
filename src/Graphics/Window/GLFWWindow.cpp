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

        GLFWwindow* window = glfwCreateWindow(size.x, size.y, name.c_str(), nullptr, nullptr);
        if (!window) {
            SR_ERROR("GLFWWindow::Initialize() : failed to create window.");
            return false;
        }

        m_window = window;

        return true;
    }
}