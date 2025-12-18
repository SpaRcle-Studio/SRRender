//
// Created by Monika on 17.05.2025.
//

//#include <Graphics/UI/UIVBoxContainerNode.h>
//
//#include <Codegen/UIVBoxContainerNode.generated.hpp>
//
//namespace SR_GRAPH_UI_NS {
//    SR_MATH_NS::FVector2 UIVBoxContainerNode::CalculateContentSize() const {
//        float_t totalHeight = 0;
//        float_t maxWidth = 0;
//
//        for (const SR_UTILS_NS::SceneObject::Ptr& pChild : GetChildrenRef()) {
//            if (pChild->GetSceneObjectType() != SR_UTILS_NS::SceneObjectType::Node) SR_UNLIKELY_ATTRIBUTE {
//                continue;
//            }
//
//            auto&& pNode = static_cast<const Node*>(pChild.Get());
//            if (pNode->GetNodeType() != SR_UTILS_NS::ECSNodeType::UIControlNode) SR_UNLIKELY_ATTRIBUTE {
//                continue;
//            }
//
//            auto&& pUIControlNode = static_cast<const UIControlNode*>(pNode);
//
//            auto&& childSize = pUIControlNode->CalculateContentSize();
//            totalHeight += childSize.y + pUIControlNode->GetLayout().margin.Vertical();
//            maxWidth = std::max(maxWidth, childSize.x + pUIControlNode->GetLayout().margin.Horizontal());
//        }
//
//        return SR_MATH_NS::FVector2(
//            maxWidth + GetLayout().padding.Horizontal(),
//            totalHeight + GetLayout().padding.Vertical()
//        );
//    }
//
//    void UIVBoxContainerNode::Layout(const SR_MATH_NS::FRect& available) {
//        Super::Layout(available);
//
//        /*// 1. Доступная область для детей = finalRect - padding
//        SR_MATH_NS::FRect childArea = m_finalRect;
//        childArea.position.x += GetLayout().padding.left;
//        childArea.position.y += GetLayout().padding.top;
//        childArea.size.x -= (GetLayout().padding.left + GetLayout().padding.right);
//        childArea.size.y -= (GetLayout().padding.top + GetLayout().padding.bottom);
//
//        /// 2. Вычисляем общую высоту всех фиксированных и процентных детей
//        float_t totalFixedHeight = 0;
//        int fillCount = 0;
//
//        for (const SR_UTILS_NS::SceneObject::Ptr& pChild : GetChildrenRef()) {
//            if (pChild->GetSceneObjectType() != SR_UTILS_NS::SceneObjectType::Node) SR_UNLIKELY_ATTRIBUTE {
//                continue;
//            }
//
//            auto&& pNode = static_cast<const Node*>(pChild.Get());
//            if (pNode->GetNodeType() != SR_UTILS_NS::ECSNodeType::UIControlNode) SR_UNLIKELY_ATTRIBUTE {
//                continue;
//            }
//
//            auto&& pUIControlNode = static_cast<const UIControlNode*>(pNode);
//
//            switch (pUIControlNode->GetLayout().heightPolicy) {
//                case UISizePolicy::Fixed:
//                    totalFixedHeight += pUIControlNode->GetLayout().height;
//                    break;
//                case UISizePolicy::Percent:
//                    totalFixedHeight += pUIControlNode->GetLayout().height * childArea.size.y;
//                    break;
//                case UISizePolicy::Content:
//                    totalFixedHeight += pUIControlNode->CalculateContentSize().y;
//                    break;
//                case UISizePolicy::Fill:
//                    ++fillCount;
//                    break;
//                default:
//                    SRHalt("Unknown UISizePolicy");
//                    break;
//            }
//
//            totalFixedHeight += pUIControlNode->GetLayout().margin.top + pUIControlNode->GetLayout().margin.bottom;
//        }
//
//        float_t remainingHeight = std::max(0.0f, childArea.size.y - totalFixedHeight);
//        float_t y = childArea.position.y;
//
//        /// 3. Расставляем детей по вертикали
//        for (SR_UTILS_NS::SceneObject::Ptr& pChild : GetChildrenRef()) {
//            if (pChild->GetSceneObjectType() != SR_UTILS_NS::SceneObjectType::Node) SR_UNLIKELY_ATTRIBUTE {
//                continue;
//            }
//
//            auto&& pNode = static_cast<Node*>(pChild.Get());
//            if (pNode->GetNodeType() != SR_UTILS_NS::ECSNodeType::UIControlNode) SR_UNLIKELY_ATTRIBUTE {
//                continue;
//            }
//
//            auto&& pUIControlNode = static_cast<UIControlNode*>(pNode);
//
//            float_t childHeight = 0;
//            switch (pUIControlNode->GetLayout().heightPolicy) {
//                case UISizePolicy::Fixed:
//                    childHeight = pUIControlNode->GetLayout().height;
//                    break;
//                case UISizePolicy::Percent:
//                    childHeight = pUIControlNode->GetLayout().height * childArea.size.y;
//                    break;
//                case UISizePolicy::Content:
//                    childHeight = pUIControlNode->CalculateContentSize().y;
//                    break;
//                case UISizePolicy::Fill:
//                    childHeight = fillCount > 0 ? remainingHeight / fillCount : 0.f;
//                    break;
//                default:
//                    SRHalt("Unknown UISizePolicy");
//                    break;
//            }
//
//            float_t childWidth = 0;
//            switch (pUIControlNode->GetLayout().widthPolicy) {
//                case UISizePolicy::Fixed:   childWidth = pUIControlNode->GetLayout().width; break;
//                case UISizePolicy::Percent: childWidth = pUIControlNode->GetLayout().width * childArea.size.x; break;
//                case UISizePolicy::Fill:    childWidth = childArea.size.x; break;
//                case UISizePolicy::Content: childWidth = pUIControlNode->CalculateContentSize().x; break;
//                default:
//                    SRHalt("Unknown UISizePolicy");
//                    break;
//            }
//
//            SR_MATH_NS::FRect childRect;
//            childRect.position = {
//                childArea.position.x + pUIControlNode->GetLayout().margin.left,
//                y + pUIControlNode->GetLayout().margin.top
//            };
//            childRect.size = {
//                childWidth,
//                childHeight
//            };
//
//            pUIControlNode->Layout(childRect);
//
//            y += childHeight + pUIControlNode->GetLayout().margin.top + pUIControlNode->GetLayout().margin.bottom;
//        }*/
//    }
//}