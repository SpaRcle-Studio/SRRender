//
// Created by Monika on 01.08.2022.
//

#ifndef SR_ENGINE_GRAPHICS_CANVAS_H
#define SR_ENGINE_GRAPHICS_CANVAS_H

#include <Graphics/stdInclude.h>

#include <Utils/ECS/Component.h>
#include <Utils/ECS/EntityRef.h>
#include <Utils/Math/Rect.h>
#include <Utils/Math/PhysicalUnit.h>

namespace SR_GRAPH_NS {
    class RenderContext;
    class RenderScene;
    class Window;
}

namespace SR_GTYPES_NS {
    class Camera;
}

namespace SR_GRAPH_UI_NS {
    SR_ENUM_NS_CLASS_T(CanvasScaleMode, uint8_t,
        ConstantPixelSize,
        ScaleWithScreenSize,
        ConstantPhysicalSize
    )

    SR_ENUM_NS_CLASS_T(CanvasScreenMatchMode, uint8_t,
        MatchWidthOrHeight,
        Expand,
        Shrink
    )

    /// @category(UI)
    class Canvas : public SR_UTILS_NS::Component {
        SR_CLASS()
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<RenderScene>;
        using Super = SR_UTILS_NS::Component;

    public:
        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }
        SR_NODISCARD SR_MATH_NS::UVector2 GetSize() const noexcept { return m_size; }
        SR_NODISCARD float_t GetScaleFactor() const noexcept { return m_scaleFactor; }

        void OnAttached() override;
        void Update(float_t dt) override;

        void SetViewportRect(const SR_MATH_NS::FRect& rect) { m_viewportRect = rect; }
        void SetScaleFactor(float_t scaleFactor);
        SR_NODISCARD const SR_MATH_NS::FRect& GetViewportRect() const noexcept { return m_viewportRect; }

        SR_NODISCARD SR_MATH_NS::FVector2 ScreenToCanvasSpace(const SR_MATH_NS::FVector2& screenPosition) const;
        SR_NODISCARD SR_MATH_NS::FRect LayoutToCanvasRect(const SR_MATH_NS::FRect& layoutRect) const;

        SR_NODISCARD SR_GTYPES_NS::Camera* GetCamera() const noexcept;
        SR_NODISCARD Window* GetWindow() const noexcept;

    private:
        SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Camera> m_camera;
        SR_MATH_NS::FRect m_viewportRect;
        float_t m_scaleFactor = 1.f;
        bool m_dirty = true;

        SR_MATH_NS::UVector2 m_size;
        RenderScenePtr m_renderScene;

    };

    /// @category(UI)
    class CanvasScaler : public SR_UTILS_NS::Component {
        SR_CLASS()
        using Super = SR_UTILS_NS::Component;
    public:
        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }
        const float_t CalculateScaleFactor(const Canvas& canvas) const;

    private:
        /// @property
        CanvasScaleMode m_scaleMode = CanvasScaleMode::ConstantPixelSize;

        /// @property @propertyCondition(This.m_scaleMode == CanvasScaleMode::ConstantPixelSize) @drag(0.1f)
        float_t m_scaleFactor = 1.f;

        /// @property @propertyCondition(This.m_scaleMode == CanvasScaleMode::ScaleWithScreenSize)
        SR_MATH_NS::UVector2 m_referenceResolution = SR_MATH_NS::UVector2(800, 600);
        /// @property @propertyCondition(This.m_scaleMode == CanvasScaleMode::ScaleWithScreenSize)
        CanvasScreenMatchMode m_screenMatchMode = CanvasScreenMatchMode::MatchWidthOrHeight;
        /// @property @propertyCondition(This.m_scaleMode == CanvasScaleMode::ScaleWithScreenSize && This.m_screenMatchMode == CanvasScreenMatchMode::MatchWidthOrHeight)
        /// @range(0.f, 1.f) @drag(0.1f)
        float_t m_match = 0.f;

        /// @property @propertyCondition(This.m_scaleMode == CanvasScaleMode::ConstantPhysicalSize)
        SR_MATH_NS::PhysicalUnit m_physicalUnit = SR_MATH_NS::PhysicalUnit::Points;
        /// @property @propertyCondition(This.m_scaleMode == CanvasScaleMode::ConstantPhysicalSize)
        float_t m_fallbackScreenDPI = 96.f;
        /// @property @propertyCondition(This.m_scaleMode == CanvasScaleMode::ConstantPhysicalSize)
        float_t m_defaultSpriteDPI = 96.f;

        /// @property @propertyCondition(This.m_scaleMode == CanvasScaleMode::ConstantPixelSize || This.m_scaleMode == CanvasScaleMode::ScaleWithScreenSize)
        float_t m_referencePixelsPerUnit = 100.f;

    };

    class IFindCanvasOwner {
    public:
        SR_NODISCARD SR_GRAPH_NS::UI::Canvas* FindCanvas(const SR_UTILS_NS::SceneObject* pSO);

    private:
        SR_UTILS_NS::EntityRef<SR_GRAPH_NS::UI::Canvas> m_canvas;

    };
}

#endif //SR_ENGINE_GRAPHICS_CANVAS_H
