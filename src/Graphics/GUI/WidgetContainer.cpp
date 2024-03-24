//
// Created by Monika on 21.03.2024.
//

#include <Graphics/GUI/WidgetContainer.h>

namespace SR_GRAPH_GUI_NS {
    WidgetContainerElement::WidgetContainerElement()
        : SR_HTYPES_NS::SharedPtr<WidgetContainerElement>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    void WidgetContainerElement::Draw() {
        const bool isActive = m_isActive && m_isActive();

        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

        if (ImGui::Button(m_text.c_str(), ImVec2(25, 25)) && m_onClick) {
            m_onClick(isActive);
        }

        ImGui::PopStyleVar();

        if (isActive) {
            ImGui::PopStyleColor();
        }
    }

    void WidgetContainer::Draw() {
        for (auto&& pElement : m_elements) {
            pElement->Draw();
        }
    }

    WidgetContainerElement& WidgetContainer::AddElement(std::string text) {
        return AddElement().SetText(std::move(text));
    }
}
