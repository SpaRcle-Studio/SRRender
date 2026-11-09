//
// Created by Monika on 21.03.2024.
//

#include <Graphics/GUI/WidgetContainer.h>

#include <ImmediateGUI/GUI/ImmediateGUI.h>

#include <Codegen/WidgetContainer.generated.hpp>

namespace SR_GRAPH_GUI_NS {
    WidgetContainerElement::WidgetContainerElement()
        : SR_HTYPES_NS::SharedPtr<WidgetContainerElement>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    void WidgetContainerElement::Draw() {
        const bool isActive = m_isActive && m_isActive();

        if (isActive) {
            SR_IMMEDIATE_GUI_NS::PushStyleColor(SR_IMMEDIATE_GUI_NS::StyleColor::Button, SR_MATH_NS::FVector4(0.45f, 0.45f, 0.45f, 1.0f));
        }

        if (m_customDraw) {
            m_customDraw(this);
        }
        else if (SR_IMMEDIATE_GUI_NS::Button(m_text.c_str(), SR_MATH_NS::FVector2(m_width, 22)) && m_onClick) {
            m_onClick(isActive);
        }


        if (isActive) {
            SR_IMMEDIATE_GUI_NS::PopStyleColor();
        }
    }

    void WidgetContainer::Draw() {
        SR_IMMEDIATE_GUI_NS::PushStyleVar(SR_IMMEDIATE_GUI_NS::StyleVar::ChildRounding, 0);
        SR_IMMEDIATE_GUI_NS::PushStyleVar(SR_IMMEDIATE_GUI_NS::StyleVar::WindowRounding, 0);
        SR_IMMEDIATE_GUI_NS::PushStyleVar(SR_IMMEDIATE_GUI_NS::StyleVar::FrameRounding, 0);
        SR_IMMEDIATE_GUI_NS::PushStyleVar(SR_IMMEDIATE_GUI_NS::StyleVar::FrameBorderSize, 1);

        for (uint32_t i = 0; i < m_elements.size(); ++i) {
            SR_IMMEDIATE_GUI_NS::PushStyleVar(SR_IMMEDIATE_GUI_NS::StyleVar::ItemSpacing,
                SR_MATH_NS::FVector2(m_elements[i]->GetItemSpacing().x, m_elements[i]->GetItemSpacing().y)
            );

            m_elements[i]->Draw();
            if (i + 1 < m_elements.size()) {
                SR_IMMEDIATE_GUI_NS::SameLine();
            }

            SR_IMMEDIATE_GUI_NS::PopStyleVar();
        }

        SR_IMMEDIATE_GUI_NS::PopStyleVar(4);
    }

    WidgetContainerElement& WidgetContainer::AddElement(std::string text) {
        return AddElement().SetText(std::move(text));
    }
}
