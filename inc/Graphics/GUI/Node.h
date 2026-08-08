//
// Created by Monika on 11.01.2022.
//

#ifndef SR_ENGINE_NODE_H
#define SR_ENGINE_NODE_H

#include <Graphics/GUI/Icons.h>

namespace SR_GRAPH_GUI_NS {
    SR_ENUM_NS_CLASS(NodeType,
        None,
        Blueprint,
        Simple,
        Tree,
        Comment,
        Houdini,
        Connector
    );

    class NodeBuilder;

    class Pin;
    class Node;
    class Link;

    class Node : public SR_UTILS_NS::NonCopyable {
    public:
        Node();
        ~Node() override;

    public:
        Node& AddInput(Pin* pin);
        Node& AddOutput(Pin* pin);

        void RemoveInput(uint32_t index);
        void RemoveOutput(uint32_t index);

        SR_NODISCARD Pin* GetInputPin(uint32_t index);
        SR_NODISCARD Pin* GetOutputPin(uint32_t index);

        SR_NODISCARD uintptr_t GetId() const;
        SR_NODISCARD std::string GetName() const;
        SR_NODISCARD uint64_t GetHashName() const;
        SR_NODISCARD bool IsConnector() const { return m_type == NodeType::Connector; }
        SR_NODISCARD const std::vector<Pin*>& GetInputs() const noexcept { return m_inputs; }
        SR_NODISCARD const std::vector<Pin*>& GetOutputs() const noexcept { return m_outputs; }
        SR_NODISCARD int32_t GetPinIndex(const Pin* pPin) const;
        SR_NODISCARD void* GetUserData() const { return m_userData; }
        SR_NODISCARD SR_MATH_NS::FRect GetRect() const { return m_rect; }

        void SetRect(const SR_MATH_NS::FRect& rect) { m_rect = rect; }

        template<typename T> SR_NODISCARD T* GetUserData() const { return reinterpret_cast<T*>(m_userData); }

        Node& SetName(std::string name);
        Node& SetPosition(const SR_MATH_NS::FVector2& pos);
        Node& SetType(NodeType type);

        void Draw(NodeBuilder* pBuilder, Pin* pNewLinkPin);
        void PostDraw();

        void SetUserData(void* pUserData) { m_userData = pUserData; }

    private:
        void* m_userData = nullptr;
        SR_MATH_NS::FRect m_rect;
        std::string m_name;
        std::vector<Pin*> m_inputs;
        std::vector<Pin*> m_outputs;
        NodeType m_type = NodeType::None;
        float_t m_maxOutputWidth = 0.f;
        bool m_hasOutputDelegates = false;

    };
}

namespace std {
    template<> struct hash<SR_GRAPH_GUI_NS::Node> {
        size_t operator()(SR_GRAPH_GUI_NS::Node const& node) const {
            std::hash<uintptr_t> h;
            return h(node.GetId()) + 0x9e3779b9 + (0 << 6) + (0 >> 2);
        }
    };

    template<> struct hash<SR_GRAPH_GUI_NS::Link> {
        size_t operator()(SR_GRAPH_GUI_NS::Node const& link) const {
            std::hash<uintptr_t> h;
            return h(link.GetId()) + 0x9e3779b9 + (0 << 6) + (0 >> 2);
        }
    };
}

#endif //SR_ENGINE_NODE_H
