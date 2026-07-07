//
// Created by Monika on 16.02.2022.
//

#ifndef SR_ENGINE_GUI_UTILS_H
#define SR_ENGINE_GUI_UTILS_H

#include <Graphics/stdInclude.h>

namespace SR_GRAPH_GUI_NS {
    class ImGuiDisabledLockGuard : public SR_UTILS_NS::NonCopyable {
    public:
        explicit ImGuiDisabledLockGuard(bool disabled);
        ~ImGuiDisabledLockGuard() override;

    private:
        bool m_disabled = false;

    };
}

#endif //SR_ENGINE_GUI_UTILS_H
