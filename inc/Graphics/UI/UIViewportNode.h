//
// Created by Monika on 18.05.2025.
//

#ifndef SR_GRAPHICS_UI_UI_VIEWPORT_NODE_H
#define SR_GRAPHICS_UI_UI_VIEWPORT_NODE_H

#include <Graphics/UI/UIContainerNode.h>

namespace SR_GRAPH_UI_NS {
    class UIViewportNode : public UIContainerNode {
        SR_CLASS()
        using Super = UIContainerNode;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<UIViewportNode>;

    public:
        void Layout(const SR_MATH_NS::FRect&) override;

    };
}

#endif //SR_GRAPHICS_UI_UI_VIEWPORT_NODE_H
