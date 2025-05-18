//
// Created by Monika on 17.05.2025.
//

#include <Graphics/UI/UIContainerNode.h>

#include <Codegen/UIContainerNode.generated.hpp>

namespace SR_GRAPH_UI_NS {
    SR_UTILS_NS::ECSNodeType UIContainerNode::GetNodeType() const noexcept {
        return SR_UTILS_NS::ECSNodeType::UIContainerNode;
    }

    void UIContainerNode::Layout(const SR_MATH_NS::FRect& available) {
        Super::Layout(available);
    }
}