//
// Created by Monika on 21.03.2024.
//

#include <Graphics/GUI/WidgetContainer.h>

#include <ImmediateGUI/GUI/ImmediateGUI.h>

namespace SR_GRAPH_GUI_NS {
    WidgetContainerElement::WidgetContainerElement()
        : SR_HTYPES_NS::SharedPtr<WidgetContainerElement>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    void WidgetContainerElement::Draw() {
        const bool isActive = m_isActive && m_isActive();

        if (isActive) {
            SR_GRAPH_GUI_NS::Immediate::PushStyleColor(SR_GRAPH_GUI_NS::Immediate::StyleColor::Button, SR_MATH_NS::FVector4(0.45f, 0.45f, 0.45f, 1.0f));
        }

        if (m_customDraw) {
            m_customDraw(this);
        }
        else if (SR_GRAPH_GUI_NS::Immediate::Button(m_text.c_str(), SR_MATH_NS::FVector2(m_width, 22)) && m_onClick) {
            m_onClick(isActive);
        }


        if (isActive) {
            SR_GRAPH_GUI_NS::Immediate::PopStyleColor();
        }
    }

    void WidgetContainer::Draw() {
        SR_GRAPH_GUI_NS::Immediate::PushStyleVar(SR_GRAPH_GUI_NS::Immediate::StyleVar::ChildRounding, 0);
        SR_GRAPH_GUI_NS::Immediate::PushStyleVar(SR_GRAPH_GUI_NS::Immediate::StyleVar::WindowRounding, 0);
        SR_GRAPH_GUI_NS::Immediate::PushStyleVar(SR_GRAPH_GUI_NS::Immediate::StyleVar::FrameRounding, 0);
        SR_GRAPH_GUI_NS::Immediate::PushStyleVar(SR_GRAPH_GUI_NS::Immediate::StyleVar::FrameBorderSize, 1);

        for (uint32_t i = 0; i < m_elements.size(); ++i) {
            SR_GRAPH_GUI_NS::Immediate::PushStyleVar(SR_GRAPH_GUI_NS::Immediate::StyleVar::ItemSpacing,
                SR_MATH_NS::FVector2(m_elements[i]->GetItemSpacing().x, m_elements[i]->GetItemSpacing().y)
            );

            m_elements[i]->Draw();
            if (i + 1 < m_elements.size()) {
                SR_GRAPH_GUI_NS::Immediate::SameLine();
            }

            SR_GRAPH_GUI_NS::Immediate::PopStyleVar();
        }

        SR_GRAPH_GUI_NS::Immediate::PopStyleVar(4);
    }

    WidgetContainerElement& WidgetContainer::AddElement(std::string text) {
        return AddElement().SetText(std::move(text));
    }
}
