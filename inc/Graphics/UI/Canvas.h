//
// Created by Monika on 01.08.2022.
//

#ifndef SR_ENGINE_GRAPHICS_CANVAS_H
#define SR_ENGINE_GRAPHICS_CANVAS_H

#include <Graphics/Render/RenderScene.h>

#include <Utils/ECS/Component.h>
#include <Utils/ECS/EntityRef.h>

namespace SR_GRAPH_NS {
    class RenderContext;
    class RenderScene;
}

namespace SR_GTYPES_NS {
    class Camera;
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

        void SetViewportRect(const SR_MATH_NS::FRect& rect) { m_viewportRect = rect; }
        SR_NODISCARD const SR_MATH_NS::FRect& GetViewportRect() const noexcept { return m_viewportRect; }

        SR_NODISCARD SR_GTYPES_NS::Camera* GetCamera() const noexcept;
        SR_NODISCARD Window* GetWindow() const noexcept;

    protected:
        void Update(float_t dt) override;

    private:
        SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Camera> m_camera;
        SR_MATH_NS::FRect m_viewportRect;

        SR_MATH_NS::UVector2 m_size;
        RenderScenePtr m_renderScene;

    };

    class IFindCanvasOwner {
    public:
        SR_NODISCARD SR_GRAPH_NS::UI::Canvas* FindCanvas(const SR_UTILS_NS::SceneObject* pSO);

    private:
        SR_UTILS_NS::EntityRef<SR_GRAPH_NS::UI::Canvas> m_canvas;

    };
}

#endif //SR_ENGINE_GRAPHICS_CANVAS_H
