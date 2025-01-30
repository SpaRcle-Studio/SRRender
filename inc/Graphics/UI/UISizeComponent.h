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

        SR_NODISCARD const bool& IsWidthChangeable() const { return m_isWidthChangeable; }
        std::vector<float> GetTest() { return m_test; }

    protected:
        void OnChanged();

    private:
        /// @property @changeCallback(OnChanged) @getter(IsWidthChangeable)
        bool m_isWidthChangeable = true;
        /// @property @changeCallback(OnChanged)
        bool m_isHeightChangeable = true;
        /// @property @changeCallback(OnChanged)
        int32_t m_width = 100;
        /// @property @changeCallback(OnChanged) @readOnly
        int8_t m_int8 = 5;
        /// @property @changeCallback(OnChanged) @drag(5)
        uint64_t m_uint64 = 500 + 400;
        /// @property @changeCallback(OnChanged)
        float_t m_height = 200.0f;
        /// @property @changeCallback(OnChanged)
        SR_MATH_NS::FSize2 m_size;
        /// @property @changeCallback(OnChanged) @drag(0.01f) @resetValue(1.0f)
        SR_MATH_NS::FVector3 m_position;
        /// @property @changeCallback(OnChanged)
        SR_MATH_NS::FVector2 m_2d;
        /// @property @changeCallback(OnChanged)
        SR_MATH_NS::BVector4 m_4db;
        /// @property @readOnly
        bool m_hasChanged = false;
        /// @property @getter(GetTest)
        std::vector<float> m_test;

    };
}
#endif //SR_GRAPHICS_UI_SIZE_COMPONENT_H
