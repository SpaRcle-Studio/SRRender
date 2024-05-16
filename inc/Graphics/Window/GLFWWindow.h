//
// Created by innerviewer on 2024-05-06.
//

#ifndef SR_ENGINE_GLFW_WINDOW_H
#define SR_ENGINE_GLFW_WINDOW_H

#include <Graphics/Window/BasicWindowImpl.h>

namespace SR_GRAPH_NS {
    class GLFWWindow : public BasicWindowImpl {
        using Super = BasicWindowImpl;
    public:
        explicit GLFWWindow()
                : Super()
        { }

    public:
        bool Initialize(const std::string& name,
                        const SR_MATH_NS::IVector2& position,
                        const SR_MATH_NS::UVector2& size,
                        bool fullScreen, bool resizable) override;

        void PollEvents() override;
    public:
        void OnSizeChangedCallback(GLFWwindow* pWindow, SR_MATH_NS::IVector2 size);
        void OnFramebufferSizeChangedCallback(GLFWwindow* pWindow, SR_MATH_NS::IVector2 size);
        void OnFocusChangedCallback(GLFWwindow* pWindow, bool isFocused);
        void OnWindowRefreshCallback(GLFWwindow* pWindow);


    public:
        SR_NODISCARD WindowType GetType() const override { return WindowType::GLFW; };
        SR_NODISCARD void* GetHandle() const override { return static_cast<void*>(m_window); };
        SR_NODISCARD GLFWwindow* GetWindow() const { return m_window; }

    private:
        GLFWwindow* m_window = nullptr;
    };
}

#endif //SR_ENGINE_GLFW_WINDOW_H
