//
// Created by Monika on 17.05.2025.
//

//#include <Graphics/UI/UIControlNode.h>
//
//#ifdef SR_RENDER_USE_YOGA
//    #include <yoga/YGEnums.h>
//    #include <yoga/YGNodeLayout.h>
//    #include <yoga/YGNodeStyle.h>
//#endif
//
//#include <Codegen/UIControlNode.generated.hpp>
//
//namespace SR_GRAPH_UI_NS {
//    void UIControlNode::Prepare(uint64_t& priority) {
//        Super::Prepare(priority);
//
//    #ifdef SR_RENDER_USE_YOGA
//        YGNodeStyleSetMarginPercent(GetYGNode(), YGEdgeLeft, m_layout.margin.left);
//        YGNodeStyleSetMarginPercent(GetYGNode(), YGEdgeTop, m_layout.margin.top);
//        YGNodeStyleSetMarginPercent(GetYGNode(), YGEdgeRight, m_layout.margin.right);
//        YGNodeStyleSetMarginPercent(GetYGNode(), YGEdgeBottom, m_layout.margin.bottom);
//
//        YGNodeStyleSetPaddingPercent(GetYGNode(), YGEdgeLeft, m_layout.padding.left);
//        YGNodeStyleSetPaddingPercent(GetYGNode(), YGEdgeTop, m_layout.padding.top);
//        YGNodeStyleSetPaddingPercent(GetYGNode(), YGEdgeRight, m_layout.padding.right);
//        YGNodeStyleSetPaddingPercent(GetYGNode(), YGEdgeBottom, m_layout.padding.bottom);
//
//        if (SR_MATH_NS::IsEquals(m_layout.aspectRatio, 0.f)) {
//            YGNodeStyleSetAspectRatio(GetYGNode(), YGUndefined);
//        }
//        else {
//            YGNodeStyleSetAspectRatio(GetYGNode(), m_layout.aspectRatio);
//        }
//
//        switch (m_layout.widthPolicy) {
//            case UISizePolicy::Auto:
//                YGNodeStyleSetWidth(GetYGNode(), YGUndefined);
//                YGNodeStyleSetWidthAuto(GetYGNode());
//                break;
//            case UISizePolicy::Fixed: YGNodeStyleSetWidth(GetYGNode(), m_layout.width); break;
//            case UISizePolicy::Percent: YGNodeStyleSetWidthPercent(GetYGNode(), m_layout.width); break;
//            default: SRHalt("Unknown UISizePolicy!"); break;
//        }
//
//        switch (m_layout.heightPolicy) {
//            case UISizePolicy::Auto:
//                YGNodeStyleSetHeight(GetYGNode(), YGUndefined);
//                YGNodeStyleSetHeightAuto(GetYGNode());
//            break;
//            case UISizePolicy::Fixed: YGNodeStyleSetHeight(GetYGNode(), m_layout.height); break;
//            case UISizePolicy::Percent: YGNodeStyleSetHeightPercent(GetYGNode(), m_layout.height); break;
//            default: SRHalt("Unknown UISizePolicy!"); break;
//        }
//
//        switch (m_layout.align) {
//            case UIAlign::Auto:         YGNodeStyleSetAlignSelf(GetYGNode(), YGAlignAuto); break;
//            case UIAlign::Stretch:      YGNodeStyleSetAlignSelf(GetYGNode(), YGAlignStretch); break;
//            case UIAlign::Center:       YGNodeStyleSetAlignSelf(GetYGNode(), YGAlignCenter); break;
//            case UIAlign::Start:        YGNodeStyleSetAlignSelf(GetYGNode(), YGAlignFlexStart); break;
//            case UIAlign::End:          YGNodeStyleSetAlignSelf(GetYGNode(), YGAlignFlexEnd); break;
//            case UIAlign::Baseline:     YGNodeStyleSetAlignSelf(GetYGNode(), YGAlignBaseline); break;
//            case UIAlign::SpaceBetween: YGNodeStyleSetAlignSelf(GetYGNode(), YGAlignSpaceBetween); break;
//            case UIAlign::SpaceAround:  YGNodeStyleSetAlignSelf(GetYGNode(), YGAlignSpaceAround); break;
//            case UIAlign::SpaceEvenly:  YGNodeStyleSetAlignSelf(GetYGNode(), YGAlignSpaceEvenly); break;
//            default: SRHalt("Unknown UIAlign!"); break;
//        }
//    #endif
//
//        //YGNodeStyleSetDirection(GetYGNode(), YGDirectionInherit);
//    }
//
//#ifdef SR_RENDER_USE_YOGA
//    float GetGlobalX(YGNodeRef node) {
//        float x = YGNodeLayoutGetLeft(node);
//        YGNodeRef parent = YGNodeGetParent(node);
//        while (parent) {
//            x += YGNodeLayoutGetLeft(parent);
//            parent = YGNodeGetParent(parent);
//        }
//        return x;
//    }
//
//    float GetGlobalY(YGNodeRef node) {
//        float y = YGNodeLayoutGetTop(node);
//        YGNodeRef parent = YGNodeGetParent(node);
//        while (parent) {
//            y += YGNodeLayoutGetTop(parent);
//            parent = YGNodeGetParent(parent);
//        }
//        return y;
//    }
//#endif
//
//    void UIControlNode::Layout(const SR_MATH_NS::FRect& available) {
//        /*/// 1. Учитываем margin
//        SR_MATH_NS::FRect inner = available;
//        inner.position.x += m_layout.margin.left - m_layout.margin.right;
//        inner.position.y += m_layout.margin.top - m_layout.margin.bottom;
//        inner.size.x -= m_layout.margin.left + m_layout.margin.right;
//        inner.size.y -= m_layout.margin.top + m_layout.margin.bottom;
//
//        auto&& contentSize = CalculateContentSize();
//
//        /// 2. Вычисляем размер
//        SR_MATH_NS::FVector2 size;
//        switch (m_layout.widthPolicy) {
//            case UISizePolicy::Fixed:   size.x = m_layout.width; break;
//            case UISizePolicy::Percent: size.x = inner.size.x * m_layout.width; break;
//            case UISizePolicy::Fill:    size.x = inner.size.x; break;
//            case UISizePolicy::Content: size.x = contentSize.x; break;
//            default: SRHalt("Unknown UISizePolicy!"); break;
//        }
//
//        switch (m_layout.heightPolicy) {
//            case UISizePolicy::Fixed:   size.y = m_layout.height; break;
//            case UISizePolicy::Percent: size.y = inner.size.y * m_layout.height; break;
//            case UISizePolicy::Fill:    size.y = inner.size.y; break;
//            case UISizePolicy::Content: size.y = contentSize.y; break;
//            default: SRHalt("Unknown UISizePolicy!"); break;
//        }
//
//        /// 3. Вычисляем позицию
//        SR_MATH_NS::FVector2 pos;
//        if (m_layout.positionType == UIPositionType::Absolute) {
//            pos = m_layout.absolutePosition;
//        }
//        else {
//            pos = inner.position + m_layout.offset;
//        }
//
//        /// 4. Устанавливаем финальные координаты
//        m_finalRect = SR_MATH_NS::FRect(pos, size);
//
//        /// 5. Layout для детей (если они есть, например в контейнере)
//        /// UIControl не имеет layout-дочерних по умолчанию*/
//
//        //YGNodeStyleSetPosition(GetYGNode(), YGEdgeLeft, available.x);
//        //YGNodeStyleSetPosition(GetYGNode(), YGEdgeTop, available.y);
//
//        //YGNodeCalculateLayout(GetYGNode(), m_viewportSize.x, m_viewportSize.y, YGDirectionLTR);
//        //YGNodeCalculateLayout(GetYGNode(), m_viewportSize.x, m_viewportSize.y, YGDirectionInherit);
//
//#ifdef SR_RENDER_USE_YOGA
//        m_hasParent = false;
//        m_parentName = SR_UTILS_NS::StringAtom();
//        if (auto&& pParent = YGNodeGetParent(GetYGNode())) {
//            m_hasParent = true;
//            m_parentName = static_cast<UINode*>(YGNodeGetContext(pParent))->GetName();
//        }
//
//        m_finalRect.x = GetGlobalX(GetYGNode());
//        m_finalRect.y = GetGlobalY(GetYGNode());
//        m_finalRect.w = YGNodeLayoutGetWidth(GetYGNode());
//        m_finalRect.h = YGNodeLayoutGetHeight(GetYGNode());
//#endif
//    }
//
//    SR_MATH_NS::FVector2 UIControlNode::CalculateContentSize() const {
//        return SR_MATH_NS::FVector2();
//    }
//
//    const UILayout& UIControlNode::GetLayout() const noexcept {
//        return m_layout;
//    }
//
//    UILayout& UIControlNode::GetLayout() noexcept {
//        return m_layout;
//    }
//
//    SR_UTILS_NS::ECSNodeType UIControlNode::GetNodeType() const noexcept {
//        return SR_UTILS_NS::ECSNodeType::UIControlNode;
//    }
//}
//