//
// Created by Monika on 17.05.2025.
//

#ifndef SR_GRAPHICS_UI_UI_V_BOX_CONTAINER_NODE_H
#define SR_GRAPHICS_UI_UI_V_BOX_CONTAINER_NODE_H

#include <Graphics/UI/UIContainerNode.h>

namespace SR_GRAPH_UI_NS {
    class SR_GRAPHICS_DLL_API UIVBoxContainerNode : public UIContainerNode {
        SR_CLASS()
        using Super = UIContainerNode;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<UIVBoxContainerNode>;

    public:
        SR_NODISCARD SR_MATH_NS::FVector2 CalculateContentSize() const override;

        void Layout(const SR_MATH_NS::FRect& available) override;

    };
}

#endif //SR_GRAPHICS_UI_UI_V_BOX_CONTAINER_NODE_H
