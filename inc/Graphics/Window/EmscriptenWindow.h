//
// Created by Monika on 01.03.2026.
//

#ifndef SR_ENGINE_RENDER_EMSCRIPTEN_WINDOW_H
#define SR_ENGINE_RENDER_EMSCRIPTEN_WINDOW_H

#include <Graphics/Window/BasicWindowImpl.h>

namespace SR_GRAPH_NS {
    class EmscriptenWindow : public BasicWindowImpl {
        using Super = BasicWindowImpl;
    public:
        explicit EmscriptenWindow()
            : Super()
        { }

    public:
        bool Initialize(const std::string& name,
                        const SR_MATH_NS::IVector2& position,
                        const SR_MATH_NS::UVector2& size,
                        bool fullScreen, bool resizable) override;

    public:
        SR_NODISCARD WindowType GetType() const override { return WindowType::Emscripten; };
        SR_NODISCARD void* GetHandle() const override { return nullptr; };

    };
}

#endif //SR_ENGINE_HEADLESS_WINDOW_H
