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
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.45f, 0.45f, 1.0f));
        }

        if (m_customDraw) {
            m_customDraw(this);
        }
        else if (ImGui::Button(m_text.c_str(), ImVec2(22, 22)) && m_onClick) {
            m_onClick(isActive);
        }

        if (isActive) {
            ImGui::PopStyleColor();
        }
    }

    void WidgetContainer::Draw() {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        for (uint32_t i = 0; i < m_elements.size(); ++i) {
            m_elements[i]->Draw();
            if (i + 1 < m_elements.size()) {
                ImGui::SameLine();
            }
        }

        ImGui::PopStyleVar(5);
    }

    WidgetContainerElement& WidgetContainer::AddElement(std::string text) {
        return AddElement().SetText(std::move(text));
    }
}
