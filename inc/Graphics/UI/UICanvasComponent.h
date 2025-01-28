//
// Created by Monika on 19.01.2025.
//

#ifndef SR_UTILS_UI_CANVAS_COMPONENT_H
#define SR_UTILS_UI_CANVAS_COMPONENT_H

#include <Utils/UI/UIModifier.h>

namespace SR_GRAPH_UI_NS {
    class UICanvasComponent : public SR_UTILS_NS::UI::UIModifierComponent {
        using Super = SR_UTILS_NS::UI::UIModifierComponent;
        SR_CLASS()
    public:
        void Prepare(SR_UTILS_NS::UI::UIModifierContext& context) const override;

    };
}

#endif //SR_UTILS_UI_CANVAS_COMPONENT_H
