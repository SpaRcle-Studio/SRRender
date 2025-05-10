//
// Created by Monika on 16.02.2022.
//

#ifndef SR_ENGINE_GUI_UTILS_H
#define SR_ENGINE_GUI_UTILS_H

#include <Graphics/macros.h>

#include <Utils/Debug.h>
#include <Utils/Math/Mathematics.h>
#include <Utils/Math/Rect.h>
#include <Utils/Types/DataStorage.h>
#include <Utils/SRLM/DataType.h>
#include <Utils/TypeTraits/Properties.h>

namespace SR_GRAPH_GUI_NS {
    class ImGuiDisabledLockGuard : public SR_UTILS_NS::NonCopyable {
    public:
        explicit ImGuiDisabledLockGuard(bool disabled);
        ~ImGuiDisabledLockGuard() override;

    private:
        bool m_disabled = false;

    };

    bool RadioButton(const char* label, bool active, float_t radius = 1.f);
}

#endif //SR_ENGINE_GUI_UTILS_H
