//
// Created by Monika on 17.05.2025.
//

#include <Graphics/UI/UIControlNode.h>

#include <Codegen/UIControlNode.generated.hpp>

namespace SR_GRAPH_UI_NS {
    void UIControlNode::Layout(const SR_MATH_NS::FRect& available) {
        /// 1. Учитываем margin
        SR_MATH_NS::FRect inner = available;
        inner.position.x += m_layout.margin.left;
        inner.position.y += m_layout.margin.top;
        inner.size.x -= (m_layout.margin.left + m_layout.margin.right);
        inner.size.y -= (m_layout.margin.top + m_layout.margin.bottom);

        auto&& contentSize = CalculateContentSize();

        /// 2. Вычисляем размер
        SR_MATH_NS::FVector2 size;
        switch (m_layout.widthPolicy) {
            case UISizePolicy::Fixed:   size.x = m_layout.width; break;
            case UISizePolicy::Percent: size.x = inner.size.x * m_layout.width; break;
            case UISizePolicy::Fill:    size.x = inner.size.x; break;
            case UISizePolicy::Content: size.x = contentSize.x; break;
            default: SRHalt("Unknown UISizePolicy!"); break;
        }

        switch (m_layout.heightPolicy) {
            case UISizePolicy::Fixed:   size.y = m_layout.height; break;
            case UISizePolicy::Percent: size.y = inner.size.y * m_layout.height; break;
            case UISizePolicy::Fill:    size.y = inner.size.y; break;
            case UISizePolicy::Content: size.y = contentSize.y; break;
            default: SRHalt("Unknown UISizePolicy!"); break;
        }

        /// 3. Вычисляем позицию
        SR_MATH_NS::FVector2 pos;
        if (m_layout.positionType == UIPositionType::Absolute) {
            pos = m_layout.absolutePosition;
        }
        else {
            pos = inner.position + m_layout.offset;
        }

        /// 4. Устанавливаем финальные координаты
        m_finalRect = SR_MATH_NS::FRect(pos, size);

        /// 5. Layout для детей (если они есть, например в контейнере)
        /// UIControl не имеет layout-дочерних по умолчанию
    }

    SR_MATH_NS::FVector2 UIControlNode::CalculateContentSize() const {
        return SR_MATH_NS::FVector2();
    }

    const UILayout& UIControlNode::GetLayout() const noexcept {
        return m_layout;
    }

    SR_UTILS_NS::ECSNodeType UIControlNode::GetNodeType() const noexcept {
        return SR_UTILS_NS::ECSNodeType::UIControlNode;
    }
}
