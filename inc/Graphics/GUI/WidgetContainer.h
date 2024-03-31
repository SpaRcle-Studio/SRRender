//
// Created by Monika on 21.03.2024.
//

#ifndef SR_ENGINE_GRAPHICS_GUI_WIDGET_CONTAINER_H
#define SR_ENGINE_GRAPHICS_GUI_WIDGET_CONTAINER_H

#include <Graphics/GUI/Widget.h>

namespace SR_GRAPH_GUI_NS {
    class WidgetContainerElement : public SR_UTILS_NS::InputHandler, public SR_HTYPES_NS::SharedPtr<WidgetContainerElement> {
        using Callback = SR_HTYPES_NS::Function<void(bool isActive)>;
        using CustomDrawCallback = SR_HTYPES_NS::Function<void(WidgetContainerElement*)>;
        using IsActiveFn = SR_HTYPES_NS::Function<bool()>;
    public:
        WidgetContainerElement();

    public:
        WidgetContainerElement& SetOnClick(Callback&& onClick) { m_onClick = std::move(onClick); return *this; }
        WidgetContainerElement& SetIsActive(IsActiveFn&& isActive) { m_isActive = std::move(isActive); return *this; }
        WidgetContainerElement& SetText(std::string text) { m_text = std::move(text); return *this; }
        WidgetContainerElement& SetCustomDraw(CustomDrawCallback&& customDraw) { m_customDraw = std::move(customDraw); return *this; }
        WidgetContainerElement& SetWidth(float_t width) { m_width = width; return *this; }
        WidgetContainerElement& SetItemSpacing(const SR_MATH_NS::FVector2& spacing) { m_itemSpacing = spacing; return *this; }

        SR_NODISCARD SR_MATH_NS::FVector2 GetItemSpacing() const noexcept { return m_itemSpacing; }

        virtual void Draw();

    private:
        SR_MATH_NS::FVector2 m_itemSpacing = { 0.f, 0.f };
        float_t m_width = 22.f;
        Callback m_onClick;
        CustomDrawCallback m_customDraw;
        IsActiveFn m_isActive;
        std::string m_text;

    };

    class WidgetContainer : public Widget {
        using Super = Widget;
    public:
        explicit WidgetContainer(std::string name)
            : Super(std::move(name))
        {
            m_elements.reserve(16);
            //AddFlags(
            //    ImGuiWindowFlags_::ImGuiWindowFlags_NoDecoration
                //ImGuiWindowFlags_::ImGuiWindowFlags_NoMove |
                //ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus |
                //ImGuiWindowFlags_::ImGuiWindowFlags_NoNavFocus |
                //ImGuiWindowFlags_::ImGuiWindowFlags_NoBackground |
                //ImGuiWindowFlags_::ImGuiWindowFlags_NoScrollWithMouse
                //ImGuiWindowFlags_::ImGuiWindowFlags_NoFocusOnAppearing |
            //);
        }

    public:
        void Draw() final;

        WidgetContainerElement& AddElement(WidgetContainerElement::Ptr pElement) { return *m_elements.emplace_back(std::move(pElement)); }
        WidgetContainerElement& AddElement() { return *m_elements.emplace_back(WidgetContainerElement::MakeShared()); }
        WidgetContainerElement& AddElement(std::string text);

    protected:
        std::vector<WidgetContainerElement::Ptr> m_elements;

    };
}

#endif //SR_ENGINE_GRAPHICS_GUI_WIDGET_CONTAINER_H
