//
// Created by Monika on 01.08.2022.
//

#ifndef SR_ENGINE_CANVAS_H
#define SR_ENGINE_CANVAS_H

#include <Utils/ECS/Component.h>
#include <Graphics/Render/RenderScene.h>

namespace SR_GRAPH_NS {
    class RenderContext;
    class RenderScene;
}

namespace SR_GRAPH_UI_NS {
    class Canvas : public SR_UTILS_NS::Component {
        SR_CLASS()
        SR_REGISTER_NEW_COMPONENT(Canvas, 1002);
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<RenderScene>;
        using RenderContextPtr = SR_HTYPES_NS::SafePtr<RenderContext>;
        using Super = SR_UTILS_NS::Component;
    public:
        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }

        void OnAttached() override;
        void OnDestroy() override;

    protected:
        void Update(float_t dt) override;

    private:
        SR_MATH_NS::UVector2 m_size;

        RenderContextPtr m_context;
        RenderScenePtr m_renderScene;

    };
}

#endif //SR_ENGINE_CANVAS_H
