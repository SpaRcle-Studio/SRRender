//
// Created by Monika on 01.08.2022.
//

#ifndef SR_ENGINE_GRAPHICS_CANVAS_H
#define SR_ENGINE_GRAPHICS_CANVAS_H

#include <Utils/ECS/Component.h>
#include <Graphics/Render/RenderScene.h>

namespace SR_GRAPH_NS {
    class RenderContext;
    class RenderScene;
}

namespace SR_GRAPH_UI_NS {
    /// @category(UI)
    class Canvas : public SR_UTILS_NS::Component {
        SR_CLASS()
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<RenderScene>;
        using Super = SR_UTILS_NS::Component;

    public:
        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }
        SR_NODISCARD SR_MATH_NS::UVector2 GetSize() const noexcept { return m_size; }

        void OnAttached() override;

    protected:
        void Update(float_t dt) override;

    private:
        SR_MATH_NS::UVector2 m_size;
        RenderScenePtr m_renderScene;

    };
}

#endif //SR_ENGINE_GRAPHICS_CANVAS_H
