//
// Created by Monika on 17.05.2025.
//

#ifndef SR_GRAPHICS_UI_UI_CONTROL_NODE_H
#define SR_GRAPHICS_UI_UI_CONTROL_NODE_H

#include <Graphics/UI/UINode.h>

namespace SR_GRAPH_UI_NS {
    SR_ENUM_NS_CLASS_T(UIPositionType, uint8_t,
        Relative, /// относительно родителя (по умолчанию)
        Absolute  /// абсолютная позиция в окне
    )

    SR_ENUM_NS_CLASS_T(UISizePolicy, uint8_t,
        Auto,
        Fixed,
        Percent
    )

    SR_ENUM_NS_CLASS_T(UIJustify, uint8_t,
        Auto,
        FlexStart,
        Center,
        FlexEnd,
        SpaceBetween,
        SpaceAround,
        SpaceEvenly
    )

    SR_ENUM_NS_CLASS_T(UIAlign, uint8_t,
        Auto,
        Start,
        Center,
        End,
        Stretch,
        Baseline,
        SpaceBetween,
        SpaceAround,
        SpaceEvenly
    )

    struct UILayout : public SR_UTILS_NS::Serializable {
        SR_STRUCT()

        /// Размеры

        /// @property
        UISizePolicy widthPolicy = UISizePolicy::Auto;
        /// @property
        UISizePolicy heightPolicy = UISizePolicy::Auto;
        /// @property
        float_t width = 0.0f; /// используется, если policy == Fixed или Percent
        /// @property
        float_t height = 0.0f;
        /// @property
        float_t aspectRatio = 0.0f;

        /// Положение

        /// @property
        UIPositionType positionType = UIPositionType::Relative;
        /// @property
        SR_MATH_NS::FVector2 absolutePosition = { 0.f, 0.f }; /// для Absolute
        /// @property
        SR_MATH_NS::FVector2 offset = { 0.f, 0.f }; /// смещение от родителя (при Relative)

        /// Внешние и внутренние отступы

        /// @property @inspector(MarginPropertyDrawer)
        SR_MATH_NS::FRect margin;
        /// @property @inspector(MarginPropertyDrawer)
        SR_MATH_NS::FRect padding;

        /// Выравнивание

        /// @property
        UIAlign align = UIAlign::Auto;
    };

    class UIControlNode : public UINode {
        SR_CLASS()
        using Super = UINode;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<UIControlNode>;

    public:
        void Prepare(uint64_t& priority) override;
        void Layout(const SR_MATH_NS::FRect& available) override;

        SR_NODISCARD SR_MATH_NS::FVector2 CalculateContentSize() const override;
        SR_NODISCARD const UILayout& GetLayout() const noexcept;
        SR_NODISCARD UILayout& GetLayout() noexcept;
        SR_NODISCARD SR_UTILS_NS::ECSNodeType GetNodeType() const noexcept override;

    private:
        /// @property @noHeader
        UILayout m_layout;

    };
}

#endif //SR_GRAPHICS_UI_UI_CONTROL_NODE_H
