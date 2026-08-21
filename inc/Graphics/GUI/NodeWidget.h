//
// Created by Monika on 14.01.2023.
//

#ifndef SR_ENGINE_EDITOR_GUI_NODE_WIDGET_H
#define SR_ENGINE_EDITOR_GUI_NODE_WIDGET_H

#include <Graphics/GUI/Widget.h>

#include <ImmediateGUI/GUI/NodeEditor.h>

#include <Utils/Serialization/Serializer.h>

namespace SR_GRAPH_GUI_NS {
    /// @abstract
    class NodeWidget : public SR_GRAPH_GUI_NS::Widget {
        SR_CLASS()
        using Super = SR_GRAPH_GUI_NS::Widget;
    public:
        NodeWidget();
        explicit NodeWidget(std::string name, SR_MATH_NS::IVector2 size = SR_MATH_NS::IVector2MAX);
        ~NodeWidget() override;

    public:
        void Init() override;

        virtual void Zoom();

    protected:
        virtual void UpdateTouch();
        virtual void DrawPopupMenu();
        virtual void DrawTopPanel();
        virtual void DrawInspectPanel();
        virtual void DrawNodeEditor();

        virtual void TopPanelSaveAt();
        virtual void TopPanelOpen();
        virtual void TopPanelSave();
        virtual void TopPanelClose();

        virtual void OnNodeTypeSelected(SR_UTILS_NS::StringAtom type, SR_MATH_NS::FVector2 pos) { }

        virtual void Execute();

        void Clear();

        void Draw() override;
        void OnClose() override;

        void BuildNodeMenu(SR_UTILS_NS::Map<SR_UTILS_NS::String, SR_UTILS_NS::Vector<SR_UTILS_NS::StringAtom>>& categories, SR_UTILS_NS::StringAtom baseClass);
        void DrawNodeMenuRecursive(const SR_UTILS_NS::Map<SR_UTILS_NS::String, SR_UTILS_NS::Vector<SR_UTILS_NS::StringAtom>>& categories, SR_UTILS_NS::StringView prefix, SR_MATH_NS::FVector2 popupPos);

    protected:
        SR_UTILS_NS::RawPointerHolder<SR_IMMEDIATE_GUI_NS::NodeEditorInstance> m_nodeGraphEditor;
        std::unique_ptr<SR_UTILS_NS::ISerializer> m_serializer;
        SR_UTILS_NS::Path m_currentFile;

        SR_UTILS_NS::Vector<SR_UTILS_NS::StringAtom> m_availableNodeTypes;
        std::string m_createNodeSearch;
        SR_UTILS_NS::Map<SR_UTILS_NS::String, SR_UTILS_NS::Vector<SR_UTILS_NS::StringAtom>> m_categories;

        float_t m_leftPaneWidth = 400.0f;
        float_t m_rightPaneWidth = 800.0f;

    };
}

#endif //SR_ENGINE_EDITOR_GUI_NODE_WIDGET_H
