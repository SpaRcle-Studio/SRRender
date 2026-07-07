//
// Created by Monika on 15.07.2023.
//

#include <Graphics/GUI/Utils.h>

#include <ImmediateGUI/GUI/ImmediateGUI.h>

namespace SR_GRAPH_GUI_NS {
    ImGuiDisabledLockGuard::ImGuiDisabledLockGuard(bool disabled)
        : SR_UTILS_NS::NonCopyable()
        , m_disabled(disabled)
    {
        if (m_disabled) {
            SR_GRAPH_GUI_NS::Immediate::BeginDisabled(true);
        }
    }

    ImGuiDisabledLockGuard::~ImGuiDisabledLockGuard() {
        if (m_disabled) {
            SR_GRAPH_GUI_NS::Immediate::EndDisabled();
        }
    }
}
