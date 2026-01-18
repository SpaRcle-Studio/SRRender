#ifndef SR_ENGINE_SDL_WINDOW_H
#define SR_ENGINE_SDL_WINDOW_H

#include <Graphics/Window/BasicWindowImpl.h>
#include <SDL3/SDL.h>

namespace SR_GRAPH_NS {

    class SDLWindow : public BasicWindowImpl {
        using Super = BasicWindowImpl;

    public:
        SDLWindow() = default;
        ~SDLWindow() override;

        bool Initialize(const std::string& name,
                        const SR_MATH_NS::IVector2& position,
                        const SR_MATH_NS::UVector2& size,
                        bool fullScreen,
                        bool resizable) override;

        void PollEvents() override;

        SR_NODISCARD WindowType GetType() const override {
            return WindowType::SDL;
        }

        SR_NODISCARD void* GetHandle() const override {
            return static_cast<void*>(m_window);
        }

        SR_NODISCARD SDL_Window* GetWindow() const {
            return m_window;
        }

    private:
        void HandleEvent(const SDL_Event& event);

    private:
        SDL_Window* m_window = nullptr;
    };
}

#endif // SR_ENGINE_SDL_WINDOW_H
