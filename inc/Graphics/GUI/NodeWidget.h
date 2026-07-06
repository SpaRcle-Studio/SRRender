//
// Created by Monika on 14.01.2023.
//

#ifndef SR_ENGINE_NODEWIDGET_H
#define SR_ENGINE_NODEWIDGET_H

#include <Graphics/GUI/NodeBuilder.h>
#include <Graphics/GUI/Widget.h>
#include <Graphics/GUI/PopupMenu.h>
#include <Utils/Resources/Xml.h>

namespace SR_SRLM_NS {
    class DataType;
}

namespace SR_GRAPH_GUI_NS {
    class INodeWidgetDataContainer {
    public:
        Node& AddNode(Node* pNode);
        Link& AddLink(Link* pLink);

        void RemoveNode(Node* pNode);
        void RemoveLink(Link* pLink);

        virtual bool CanAddNode(Node* pNode) { return true; }

        void ClearContainer();

    protected:
        std::map<uintptr_t, Node*> m_nodes;
        std::map<uintptr_t, Link*> m_links;

    };

    /// @abstract
    class NodeWidget : public SR_GRAPH_GUI_NS::Widget, public INodeWidgetDataContainer {
        SR_CLASS()
        using Super = SR_GRAPH_GUI_NS::Widget;
    public:
        NodeWidget() = default;
        explicit NodeWidget(std::string name, SR_MATH_NS::IVector2 size = SR_MATH_NS::IVector2MAX);
        ~NodeWidget() override;

    public:
        void Init() override;

        virtual void Zoom() { }

    protected:
        virtual void UpdateTouch();
        virtual void DrawPopupMenu();
        virtual void DrawTopPanel();
        virtual void DrawLeftPanel();
        virtual void DrawNodeEditor();

        virtual void TopPanelSaveAt();
        virtual void TopPanelOpen();
        virtual void TopPanelSave();
        virtual void TopPanelClose();

        virtual void Execute();

        void Clear();

        void Draw() override;
        void OnClose() override;

    protected:
        void* m_editor = nullptr;

        SR_UTILS_NS::Path m_currentFile;

        float_t m_leftPaneWidth = 400.0f;
        float_t m_rightPaneWidth = 800.0f;

    };
}

#endif //SR_ENGINE_NODEWIDGET_H
