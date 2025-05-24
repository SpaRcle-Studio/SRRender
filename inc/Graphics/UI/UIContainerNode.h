//
// Created by Monika on 17.05.2025.
//

#ifndef SR_GRAPHICS_UI_UI_CONTAINER_NODE_H
#define SR_GRAPHICS_UI_UI_CONTAINER_NODE_H

#include <Graphics/UI/UIControlNode.h>

namespace SR_GRAPH_UI_NS {
    SR_ENUM_NS_CLASS_T(UIContainerDirection, uint8_t,
        Row, Column, RowReverse, ColumnReverse
    )

    class UIContainerNode : public UIControlNode {
        SR_CLASS()
        using Super = UIControlNode;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<UIContainerNode>;

    public:
        SR_NODISCARD SR_UTILS_NS::ECSNodeType GetNodeType() const noexcept override;

        void Prepare(uint64_t& priority) override;
        void Layout(const SR_MATH_NS::FRect& available) override;

    private:
        /// @property
        UIContainerDirection m_direction = UIContainerDirection::Row;
        /// @property
        UIAlign m_containerAlign = UIAlign::Auto;
        /// @property
        UIJustify m_justify = UIJustify::Auto;

    };
}

#endif //SR_GRAPHICS_UI_UI_CONTAINER_NODE_H
