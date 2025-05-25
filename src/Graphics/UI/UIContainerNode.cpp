//
// Created by Monika on 17.05.2025.
//

#include <Graphics/UI/UIContainerNode.h>

#include <Enum/UIContainerDirection.hpp>

#include <Codegen/UIContainerNode.generated.hpp>

namespace SR_GRAPH_UI_NS {
    SR_UTILS_NS::ECSNodeType UIContainerNode::GetNodeType() const noexcept {
        return SR_UTILS_NS::ECSNodeType::UIContainerNode;
    }

    void UIContainerNode::Prepare(uint64_t& priority) {
        SR_TRACY_ZONE;

        Super::Prepare(priority);

        switch (m_containerAlign) {
            case UIAlign::Auto:         YGNodeStyleSetAlignItems(GetYGNode(), YGAlignAuto); break;
            case UIAlign::Stretch:      YGNodeStyleSetAlignItems(GetYGNode(), YGAlignStretch); break;
            case UIAlign::Center:       YGNodeStyleSetAlignItems(GetYGNode(), YGAlignCenter); break;
            case UIAlign::Start:        YGNodeStyleSetAlignItems(GetYGNode(), YGAlignFlexStart); break;
            case UIAlign::End:          YGNodeStyleSetAlignItems(GetYGNode(), YGAlignFlexEnd); break;
            case UIAlign::Baseline:     YGNodeStyleSetAlignItems(GetYGNode(), YGAlignBaseline); break;
            case UIAlign::SpaceBetween: YGNodeStyleSetAlignItems(GetYGNode(), YGAlignSpaceBetween); break;
            case UIAlign::SpaceAround:  YGNodeStyleSetAlignItems(GetYGNode(), YGAlignSpaceAround); break;
            case UIAlign::SpaceEvenly:  YGNodeStyleSetAlignItems(GetYGNode(), YGAlignSpaceEvenly); break;
            default: SRHalt("Unknown UIAlign!"); break;
        }

        switch (m_justify) {
            case UIJustify::Auto:
            case UIJustify::FlexStart:    YGNodeStyleSetJustifyContent(GetYGNode(), YGJustifyFlexStart); break;
            case UIJustify::Center:       YGNodeStyleSetJustifyContent(GetYGNode(), YGJustifyCenter); break;
            case UIJustify::FlexEnd:      YGNodeStyleSetJustifyContent(GetYGNode(), YGJustifyFlexEnd); break;
            case UIJustify::SpaceBetween: YGNodeStyleSetJustifyContent(GetYGNode(), YGJustifySpaceBetween); break;
            case UIJustify::SpaceAround:  YGNodeStyleSetJustifyContent(GetYGNode(), YGJustifySpaceAround); break;
            case UIJustify::SpaceEvenly:  YGNodeStyleSetJustifyContent(GetYGNode(), YGJustifySpaceEvenly); break;
            default: SRHalt("Unknown UIJustify!"); break;
        }

        switch (m_direction) {
            case UIContainerDirection::Row:
                YGNodeStyleSetFlexDirection(GetYGNode(), YGFlexDirectionRow);
                break;
            case UIContainerDirection::Column:
                YGNodeStyleSetFlexDirection(GetYGNode(), YGFlexDirectionColumn);
                break;
            case UIContainerDirection::RowReverse:
                YGNodeStyleSetFlexDirection(GetYGNode(), YGFlexDirectionRowReverse);
                break;
            case UIContainerDirection::ColumnReverse:
                YGNodeStyleSetFlexDirection(GetYGNode(), YGFlexDirectionColumnReverse);
                break;
            default:
                SRHalt("UIContainerNode::Prepare() : invalid direction \"{}\"!", m_direction);
                break;
        }

        //YGNodeSetAlwaysFormsContainingBlock(GetYGNode(), true /*alwaysFormsContainingBlock*/);

        for (auto&& pChild : GetChildrenRef()) {
            if (auto&& pNode = dynamic_cast<UINode*>(pChild.Get())) {
                pNode->Prepare(priority);
            }
        }
    }

    void UIContainerNode::Layout(const SR_MATH_NS::FRect& available) {
        SR_TRACY_ZONE;

        Super::Layout(available);

        for (auto&& pChild : GetChildrenRef()) {
            if (auto&& pNode = dynamic_cast<UINode*>(pChild.Get())) {
                pNode->Layout(available);
            }
        }
    }
}