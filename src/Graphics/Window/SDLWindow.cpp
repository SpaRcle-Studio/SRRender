#include <Graphics/Window/SDLWindow.h>
#include <SDL3/SDL_vulkan.h>

#include <Utils/Profile/TracyContext.h>

// Helper: convert SDL event type (Uint32) to a human-readable string.
static std::string SDLEventTypeToString(Uint32 type) {
    switch (type) {
        case SDL_EVENT_QUIT: return "SDL_EVENT_QUIT";
        case SDL_EVENT_WINDOW_RESIZED: return "SDL_EVENT_WINDOW_RESIZED";
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: return "SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED";
        case SDL_EVENT_WINDOW_FOCUS_GAINED: return "SDL_EVENT_WINDOW_FOCUS_GAINED";
        case SDL_EVENT_WINDOW_FOCUS_LOST: return "SDL_EVENT_WINDOW_FOCUS_LOST";
        case SDL_EVENT_MOUSE_WHEEL: return "SDL_EVENT_MOUSE_WHEEL";
        case SDL_EVENT_KEY_DOWN: return "SDL_EVENT_KEY_DOWN";
        case SDL_EVENT_KEY_UP: return "SDL_EVENT_KEY_UP";
        case SDL_EVENT_MOUSE_MOTION: return "SDL_EVENT_MOUSE_MOTION";
        case SDL_EVENT_MOUSE_BUTTON_DOWN: return "SDL_EVENT_MOUSE_BUTTON_DOWN";
        case SDL_EVENT_MOUSE_BUTTON_UP: return "SDL_EVENT_MOUSE_BUTTON_UP";
        default: return "SDL_EVENT_UNKNOWN(" + std::to_string(type) + ")";
    }
}

namespace SR_GRAPH_NS {
    SDLWindow::~SDLWindow() {
        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }

        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    bool SDLWindow::Initialize(
        const std::string& name,
        const SR_MATH_NS::IVector2& position,
        const SR_MATH_NS::UVector2& size,
        bool fullScreen,
        bool resizable
    ) {
        SR_LOG("SDLWindow::Initialize() : initializing SDL window...");

        SDL_Init(SDL_INIT_VIDEO);

        std::string drivers = "";
        for (int i = 0; i < SDL_GetNumVideoDrivers(); ++i) {
            drivers += SDL_GetVideoDriver(i) + std::string(" ");
        }

        SR_LOG("SDLWindow::Initialize() : supported SDL video drivers: \n\t{}", drivers);

        // Check if Vulkan is supported
        if (!SDL_Vulkan_LoadLibrary(nullptr)) {
            SR_ERROR("SDLWindow::Initialize() : failed to load Vulkan library: {}" + std::string(SDL_GetError()));
            return false;
        }

        SR_LOG("SDLWindow::Initialize() : Vulkan library loaded successfully");


        //
        // if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        //     SR_ERROR("SDLWindow::Initialize() : SDL subsystem init failed: \n{}", SDL_GetError());
        //     return false;
        // }

        Uint32 flags = SDL_WINDOW_VULKAN;

        if (resizable)
            flags |= SDL_WINDOW_RESIZABLE;
        if (fullScreen)
            flags |= SDL_WINDOW_FULLSCREEN;

        m_window = SDL_CreateWindow(
            name.c_str(),
            size.x, size.y,
            flags
        );

        if (!m_window) {
            SR_ERROR("SDLWindow::Initialize() : SDL window creation failed: \n{}", SDL_GetError());
            return false;
        }

        // TODO: Move window to position?

        int w = -1;
        int h = -1;
        SDL_GetWindowSize(m_window, &w, &h);

        if (w != -1 && h != -1) {
            m_size = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };
            m_surfaceSize = m_size;
        }

        m_isValid = true;
        m_isInitialized = true;

        return true;
    }

    void SDLWindow::PollEvents() {
        SR_TRACY_ZONE;

        SDL_PumpEvents();  // Collect OS events

        SDL_Event event;
        while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST)) {
            HandleEvent(event);
            // TODO: filter events by window ID
        }
    }

    void SDLWindow::HandleEvent(const SDL_Event& event) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                m_isValid = false;
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                m_size = { static_cast<uint32_t>(event.window.data1), static_cast<uint32_t>(event.window.data2) };
                m_surfaceSize = { static_cast<uint32_t>(event.window.data1), static_cast<uint32_t>(event.window.data2) };

                m_resizeCallback(this, event.window.data1, event.window.data2);
                break;

            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                break;

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                m_focusCallback(this, true);
                break;

            case SDL_EVENT_WINDOW_FOCUS_LOST:
                m_focusCallback(this, false);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                m_scrollCallback(this, event.wheel.x, event.wheel.y);
                break;

            default:
                break;
        }

        // Log the event type for debugging
        SR_LOG("SDLWindow::HandleEvent() : event type = {}", SDLEventTypeToString(event.type));
    }

}
