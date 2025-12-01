//
// Created by innerviewer on 2024-05-06.
//

#ifndef SR_ENGINE_HEADLESS_WINDOW_H
#define SR_ENGINE_HEADLESS_WINDOW_H

#include <Graphics/Window/BasicWindowImpl.h>

namespace SR_GRAPH_NS {
    class HeadlessWindow : public BasicWindowImpl {
        using Super = BasicWindowImpl;
    public:
        explicit HeadlessWindow()
            : Super()
        { }

    public:
        bool Initialize(const std::string& name,
                        const SR_MATH_NS::IVector2& position,
                        const SR_MATH_NS::UVector2& size,
                        bool fullScreen, bool resizable) override;

    public:
        SR_NODISCARD WindowType GetType() const override { return WindowType::Headless; };
        SR_NODISCARD void* GetHandle() const override { return nullptr; };

    };
}

#endif //SR_ENGINE_HEADLESS_WINDOW_H
