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
        UIViewportNode();

    public:
        void Layout(const SR_MATH_NS::FRect&) override;

    private:
        SR_UTILS_NS::Subscription m_onEngineUpdate;
        SR_UTILS_NS::Subscription m_keyDown;

        /// @property
        bool m_manualLayoutRecalc = false;
        /// @property @dontSave
        bool m_doRecalcLayout = false;

    };
}

#endif //SR_GRAPHICS_UI_UI_VIEWPORT_NODE_H
