//
// Created by Monika on 19.01.2025.
//

#ifndef SR_GRAPHICS_UI_SIZE_COMPONENT_H
#define SR_GRAPHICS_UI_SIZE_COMPONENT_H

#include <Utils/UI/UIModifier.h>

namespace SR_GRAPH_UI_NS {
    class UISizeComponent : public SR_UTILS_NS::UI::UIModifierComponent {
        using Super = SR_UTILS_NS::UI::UIModifierComponent;
        SR_REGISTER_NEW_COMPONENT(UISizeComponent, 1000);
        SR_CLASS()
    public:
        void Prepare(SR_UTILS_NS::UI::UIModifierContext& context) const override;

        const bool& IsWidthChangeable() const { return m_isWidthChangeable; }
        std::vector<float> GetTest() { return m_test; }

    protected:
        void OnChanged();

    private:
        /// @property @changeCallback(OnChanged) @getter(IsWidthChangeable)
        bool m_isWidthChangeable = true;
        /// @property @changeCallback(OnChanged)
        bool m_isHeightChangeable = true;
        /// @property @changeCallback(OnChanged)
        SR_MATH_NS::FSize2 m_size;
        /// @property @changeCallback(OnChanged) @drag(0.1f) @resetValue(SR_MATH_NS::FVector3::Zero())
        SR_MATH_NS::FVector3 m_position;
        /// @property @changeCallback(OnChanged) @drag(0.1f) @resetValue(SR_MATH_NS::FVector3::Zero())
        SR_MATH_NS::FVector2 m_2d;
        /// @property @changeCallback(OnChanged) @drag(0.1f) @resetValue(SR_MATH_NS::FVector3::Zero())
        SR_MATH_NS::BVector4 m_4db;
        /// @property @readOnly
        bool m_hasChanged = false;
        /// @property @getter(GetTest)
        std::vector<float> m_test;

    };
}
#endif //SR_GRAPHICS_UI_SIZE_COMPONENT_H
