//
// Created by Monika on 19.01.2025.
//

#ifndef SR_GRAPHICS_UI_SIZE_COMPONENT_H
#define SR_GRAPHICS_UI_SIZE_COMPONENT_H

#include <Utils/UI/UIModifier.h>

namespace SR_GRAPH_UI_NS {
    class UISizeComponent : public SR_UTILS_NS::UI::UIModifierComponent {
        using Super = SR_UTILS_NS::UI::UIModifierComponent;
        SR_REGISTER_NEW_COMPONENT(UISizeComponent, 1001);
        SR_CLASS()
    public:
        void Prepare(SR_UTILS_NS::UI::UIModifierContext& context) const override;

    protected:
        void OnChanged();

    private:
        /// @property @changeCallback(OnChanged)
        bool m_isWidthChangeable = true;
        /// @property @changeCallback(OnChanged)
        bool m_isHeightChangeable = true;

    };
}

#endif //SR_GRAPHICS_UI_SIZE_COMPONENT_H
