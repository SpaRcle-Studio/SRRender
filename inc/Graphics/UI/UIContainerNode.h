//
// Created by Monika on 17.05.2025.
//

#ifndef SR_GRAPHICS_UI_UI_CONTAINER_NODE_H
#define SR_GRAPHICS_UI_UI_CONTAINER_NODE_H

#include <Graphics/UI/UIControlNode.h>

namespace SR_GRAPH_UI_NS {
    class UIContainerNode : public UIControlNode {
        SR_CLASS()
        using Super = UIControlNode;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<UIContainerNode>;

    public:
        SR_NODISCARD SR_UTILS_NS::ECSNodeType GetNodeType() const noexcept override;

        void Layout(const SR_MATH_NS::FRect& available) override;

    };
}

#endif //SR_GRAPHICS_UI_UI_CONTAINER_NODE_H
