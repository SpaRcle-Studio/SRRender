//
// Created by Monika on 17.05.2025.
//

#include <Graphics/UI/UIHBoxContainerNode.h>

#include <Codegen/UIHBoxContainerNode.generated.hpp>

namespace SR_GRAPH_UI_NS {
    SR_MATH_NS::FVector2 UIHBoxContainerNode::CalculateContentSize() const {
        float_t totalWidth = 0;
        float_t maxHeight = 0;

        for (const SR_UTILS_NS::SceneObject::Ptr& pChild : GetChildrenRef()) {
            if (pChild->GetSceneObjectType() != SR_UTILS_NS::SceneObjectType::Node || !pChild->IsActive()) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            auto&& pUIControlNode = dynamic_cast<const UIControlNode*>(pChild.Get());
            if (!pUIControlNode) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            auto&& childSize = pUIControlNode->CalculateContentSize();
            totalWidth += childSize.x + pUIControlNode->GetLayout().margin.Horizontal();
            maxHeight = std::max(maxHeight, childSize.y + pUIControlNode->GetLayout().margin.Vertical());
        }

        return SR_MATH_NS::FVector2(
            totalWidth + GetLayout().padding.Horizontal(),
            maxHeight + GetLayout().padding.Vertical()
        );
    }

    void UIHBoxContainerNode::Layout(const SR_MATH_NS::FRect& available) {
        Super::Layout(available);

        /*/// 1. Доступная область для детей = finalRect - padding
        SR_MATH_NS::FRect childArea = m_finalRect;
        //childArea.position.x += GetLayout().padding.left;
        //childArea.position.y += GetLayout().padding.top;
        //childArea.size.x -= (GetLayout().padding.left + GetLayout().padding.right);
        //childArea.size.y -= (GetLayout().padding.top + GetLayout().padding.bottom);

        /// 2. Вычисляем общую ширину всех фиксированных и процентных детей
        float_t totalFixedWidth = 0;
        int fillCount = 0;

        for (const SR_UTILS_NS::SceneObject::Ptr& pChild : GetChildrenRef()) {
            if (pChild->GetSceneObjectType() != SR_UTILS_NS::SceneObjectType::Node || !pChild->IsActive()) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            auto&& pUIControlNode = dynamic_cast<const UIControlNode*>(pChild.Get());
            if (!pUIControlNode) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            switch (pUIControlNode->GetLayout().widthPolicy) {
                case UISizePolicy::Fixed:
                    totalFixedWidth += pUIControlNode->GetLayout().width;
                    break;
                case UISizePolicy::Percent:
                    totalFixedWidth += pUIControlNode->GetLayout().width * childArea.size.x;
                    break;
                case UISizePolicy::Content:
                    totalFixedWidth += pUIControlNode->CalculateContentSize().x;
                    break;
                case UISizePolicy::Fill:
                    ++fillCount;
                    break;
                default:
                    SRHalt("Unknown UISizePolicy");
                    break;
            }

            totalFixedWidth += pUIControlNode->GetLayout().margin.left + pUIControlNode->GetLayout().margin.right;
        }

        //float_t remainingWidth = std::max(0.0f, childArea.size.x - totalFixedWidth);
        float_t remainingWidth = m_finalRect.w;
        float_t x = childArea.position.x;

        /// 3. Расставляем детей по горизонтали
        uint32_t childIndex = 0;
        for (SR_UTILS_NS::SceneObject::Ptr& pChild : GetChildrenRef()) {
            if (pChild->GetSceneObjectType() != SR_UTILS_NS::SceneObjectType::Node || !pChild->IsActive()) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            auto&& pUIControlNode = dynamic_cast<UIControlNode*>(pChild.Get());
            if (!pUIControlNode) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            ++childIndex;

            float_t childWidth = 0;
            switch (pUIControlNode->GetLayout().widthPolicy) {
                case UISizePolicy::Fixed:
                    childWidth = pUIControlNode->GetLayout().width;
                    break;
                case UISizePolicy::Percent:
                    childWidth = pUIControlNode->GetLayout().width * childArea.size.x;
                    break;
                case UISizePolicy::Content:
                    childWidth = pUIControlNode->CalculateContentSize().x;
                    break;
                case UISizePolicy::Fill:
                    childWidth = fillCount > 0 ? remainingWidth / fillCount : 0.f;
                    break;
                default:
                    SRHalt("Unknown UISizePolicy");
                    break;
            }

            float childHeight = 0;
            switch (pUIControlNode->GetLayout().heightPolicy) {
                case UISizePolicy::Fixed:   childHeight = pUIControlNode->GetLayout().height; break;
                case UISizePolicy::Percent: childHeight = pUIControlNode->GetLayout().height * childArea.size.y; break;
                case UISizePolicy::Fill:    childHeight = childArea.size.y; break;
                case UISizePolicy::Content: childHeight = pUIControlNode->CalculateContentSize().y; break;
                default:
                    SRHalt("Unknown UISizePolicy");
                    break;
            }

            SR_MATH_NS::FRect childRect;
            childRect.position = { x + pUIControlNode->GetLayout().margin.left, childArea.position.y + pUIControlNode->GetLayout().margin.top };
            childRect.size = { childWidth, childHeight };

            pUIControlNode->Layout(childRect);
            //pUIControlNode->Layout(SR_MATH_NS::FRect(0 + 400 * childIndex, 0, 200, 200));

            x += childWidth + pUIControlNode->GetLayout().margin.left + pUIControlNode->GetLayout().margin.right;
        }*/

        uint32_t childCount = 0;

        for (SR_UTILS_NS::SceneObject::Ptr& pChild : GetChildrenRef()) {
            if (pChild->GetSceneObjectType() != SR_UTILS_NS::SceneObjectType::Node || !pChild->IsActive()) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            auto&& pUIControlNode = dynamic_cast<UIControlNode*>(pChild.Get());
            if (!pUIControlNode) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            ++childCount;
        }

        if (childCount == 0) {
            return;
        }

        //uint32_t childIndex = 0;
        float_t childPosX = m_finalRect.position.x;

        for (SR_UTILS_NS::SceneObject::Ptr& pChild : GetChildrenRef()) {
            if (pChild->GetSceneObjectType() != SR_UTILS_NS::SceneObjectType::Node || !pChild->IsActive()) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            auto&& pUIControlNode = dynamic_cast<UIControlNode*>(pChild.Get());
            if (!pUIControlNode) SR_UNLIKELY_ATTRIBUTE {
                continue;
            }

            //++childIndex;

            float_t childWidth = m_finalRect.w / static_cast<float_t>(childCount);

            SR_MATH_NS::FRect childRect;
            childRect.position = { childPosX, m_finalRect.y };
            childRect.size = { childWidth, m_finalRect.h };

            pUIControlNode->Layout(childRect);

            childPosX += childWidth * 2.f;
        }
    }
}