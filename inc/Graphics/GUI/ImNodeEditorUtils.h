//
// Created by Monika on 14.01.2023.
//

#ifndef SR_ENGINE_IMNODEEDITORUTILS_H
#define SR_ENGINE_IMNODEEDITORUTILS_H

#ifdef SR_USE_IMGUI_NODE_EDITOR
    #include <imgui-node-editor/imgui_node_editor.h>
#endif

namespace SR_SRLM_NS {
    enum class DataTypeClass : uint8_t;
}

namespace SR_GRAPH_GUI_NS {
    class Pin;

    bool IsPinsCompatible(SR_SRLM_NS::DataTypeClass first, SR_SRLM_NS::DataTypeClass second);

    bool CanCreateLink(Pin* a, Pin* b);

#ifdef SR_USE_IMGUI_NODE_EDITOR
    struct ImNodeIdLess {
        bool operator()(const ax::NodeEditor::NodeId& lhs, const ax::NodeEditor::NodeId& rhs) const {
            return lhs.AsPointer() < rhs.AsPointer();
        }
    };

    typedef std::map<ax::NodeEditor::NodeId, float, ImNodeIdLess> NodesTouchTimes;
#endif
}

#endif //SR_ENGINE_IMNODEEDITORUTILS_H
