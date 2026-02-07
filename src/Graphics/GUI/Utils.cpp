//
// Created by Monika on 15.07.2023.
//

#include <Graphics/GUI/Utils.h>
#include <Graphics/GUI/ImGUI.h>

namespace SR_GRAPH_GUI_NS {
    ImGuiDisabledLockGuard::ImGuiDisabledLockGuard(bool disabled)
        : SR_UTILS_NS::NonCopyable()
        , m_disabled(disabled)
    {
        if (m_disabled) {
            ImGui::BeginDisabled(true);
        }
    }

    ImGuiDisabledLockGuard::~ImGuiDisabledLockGuard() {
        if (m_disabled) {
            ImGui::EndDisabled();
        }
    }

    bool RadioButton(const char* label, bool active, float_t radius) {
        SR_TRACY_ZONE;
    
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
        
        const float square_sz = ImGui::GetFrameHeight() * radius;
        const ImVec2 pos = window->DC.CursorPos;
        
        // Precalculate flags and dimensions
        const bool has_label = label_size.x > 0.0f;
        const float label_offset = has_label ? style.ItemInnerSpacing.x + label_size.x : 0.0f;
        const float padding_scaled = style.FramePadding.y / 1.75f;
        
        const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
        const ImRect total_bb(pos, pos + ImVec2(square_sz + label_offset, 
                            (label_size.y + style.FramePadding.y * 2.0f) / 1.75f));
        
        ImGui::ItemSize(total_bb, padding_scaled);
        if (!ImGui::ItemAdd(total_bb, id))
            return false;

        // Calculate center once
        const float half_square = square_sz * 0.5f;
        ImVec2 center = ImVec2(pos.x + half_square, pos.y + half_square);
        center.x = IM_ROUND(center.x);
        center.y = IM_ROUND(center.y);
        
        const float radiusInternal = (square_sz - 1.0f) * 0.5f;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
        
        if (pressed)
            ImGui::MarkItemEdited(id);

        ImGui::RenderNavHighlight(total_bb, id);
        
        // Determine segment count based on size
        const int num_segments = 4;
        
        // Calculate background color once
        const ImGuiCol frame_col = (held && hovered) ? ImGuiCol_FrameBgActive 
                                : hovered ? ImGuiCol_FrameBgHovered 
                                : ImGuiCol_FrameBg;
        
        window->DrawList->AddCircleFilled(center, radiusInternal, 
                                        ImGui::GetColorU32(frame_col), num_segments);

        if (active) {
            const float pad = ImMax(1.0f, IM_FLOOR(square_sz / 6.0f));
            window->DrawList->AddCircleFilled(center, radiusInternal - pad, 
                                            ImGui::GetColorU32(ImGuiCol_CheckMark), num_segments);
        }

        if (style.FrameBorderSize > 0.0f) {
            window->DrawList->AddCircle(center + ImVec2(1, 1), radiusInternal, 
                                        ImGui::GetColorU32(ImGuiCol_BorderShadow), 
                                        num_segments, style.FrameBorderSize);
            window->DrawList->AddCircle(center, radiusInternal, 
                                        ImGui::GetColorU32(ImGuiCol_Border), 
                                        num_segments, style.FrameBorderSize);
        }

        // Only process label if needed
        if (has_label || g.LogEnabled) {
            ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, 
                                    check_bb.Min.y + style.FramePadding.y);
            if (g.LogEnabled)
                ImGui::LogRenderedText(&label_pos, active ? "(x)" : "( )");
            if (has_label)
                ImGui::RenderText(label_pos, label);
        }

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
        return pressed;
    }
}
