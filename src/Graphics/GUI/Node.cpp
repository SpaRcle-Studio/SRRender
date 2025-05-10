//
// Created by Monika on 11.01.2022.
//

#include <Graphics/GUI/Node.h>
#include <Graphics/GUI/NodeManager.h>
#include <Graphics/GUI/Pin.h>
#include <Graphics/GUI/NodeBuilder.h>
#include <Utils/SRLM/DataType.h>
#include <Utils/SRLM/LogicalNodes.h>
#include <Utils/Common/HashManager.h>

namespace SR_GRAPH_GUI_NS {
    Node::Node()
       // : Node(std::string(), NodeType::None, ImColor(255, 255, 255, 255))
    { }

    /*Node::Node(SR_SRLM_NS::LogicalNode* pNode)
        : m_logicalNode(pNode)
    {
        m_name = pNode->GetNodeName();

        switch (pNode->GetType()) {
            case SR_SRLM_NS::LogicalNodeType::Compute:
                m_type = NodeType::Simple;
                break;
            case SR_SRLM_NS::LogicalNodeType::Connector:
                m_type = NodeType::Connector;
                break;
            case SR_SRLM_NS::LogicalNodeType::Executable:
                m_type = NodeType::Blueprint;
                break;
            default:
                SRHaltOnce("Unknown node type! Type: " + SR_UTILS_NS::EnumReflector::ToStringAtom(pNode->GetType()).ToStringRef());
                break;
        }

        for (auto&& pin : pNode->GetInputs()) {
            AddInput(new Pin(SR_HASH_TO_STR(pin.hashName).data(), pin.pData));
        }

        for (auto&& pin : pNode->GetOutputs()) {
            AddOutput(new Pin(SR_HASH_TO_STR(pin.hashName).data(), pin.pData));
        }
    }*/

  // Node::Node(const std::string& name)
  //     : Node(name, NodeType::None, ImColor(255, 255, 255, 255))
  // { }

  // Node::Node(const std::string& name, NodeType type)
  //     : Node(name, type, ImColor(255, 255, 255, 255))
  // { }

  // Node::Node(const std::string& name, ImColor color)
  //     : Node(name, NodeType::None, color)
  // { }

  // Node::Node(std::string  name, NodeType type, ImColor color)
  //     : m_name(std::move(name))
  //     , m_color(color)
  //     , m_type(type)
  // { }

    Node& Node::AddInput(Pin *pin) {
        pin->m_kind = PinKind::Input;
        pin->SetNode(this);

        m_inputs.emplace_back(pin);

        return *this;
    }

    Node& Node::AddOutput(Pin* pin) {
        pin->m_kind = PinKind::Output;
        pin->SetNode(this);

        if (const auto pinWidth = pin->GetWidth(); pinWidth > m_maxOutputWidth) {
            m_maxOutputWidth = pinWidth;
        }

        /// if (pin->GetType() == PinType::Delegate) {
        ///     m_hasOutputDelegates = true;
        /// }

        m_outputs.emplace_back(pin);

        return *this;
    }

    void Node::Draw(NodeBuilder* pBuilder, Pin* pNewLinkPin) {

    }

    void Node::PostDraw() {
        for (auto&& pPin : m_inputs) {
            pPin->PostDrawOption();
        }
    }

    uintptr_t Node::GetId() const {
        /// TODO: переделать, при сохранении будут проблемы
        return reinterpret_cast<const uintptr_t>(this);
    }

    Node::~Node() {
        for (auto& pin : m_inputs)
            delete pin;

        for (auto& pin : m_outputs)
            delete pin;

        m_inputs.clear();
        m_outputs.clear();
    }

    Pin* Node::GetInputPin(uint32_t index) {
        if (m_inputs.size() <= index) {
            SRAssert(false);
            return nullptr;
        }
        return m_inputs.at(index);
    }

    Pin* Node::GetOutputPin(uint32_t index) {
        if (m_outputs.size() <= index) {
            SRAssert(false);
            return nullptr;
        }
        return m_outputs.at(index);
    }

    Node& Node::SetPosition(const SR_MATH_NS::FVector2& pos) {

        return *this;
    }

    Node& Node::SetName(std::string name) {
        m_name = std::move(name);
        return *this;
    }

    Node& Node::SetType(NodeType type) {
        m_type = type;
        return *this;
    }

    std::string Node::GetName() const {
        return m_name;
    }

    /*Node& Node::AddInput(PinType type) {
        return AddInput(new Pin(std::string(), SR_SRLM_NS::DataTypeAllocator::Instance().Allocate(type)));
    }

    Node& Node::AddOutput(PinType type) {
        return AddOutput(new Pin(std::string(), SR_SRLM_NS::DataTypeAllocator::Instance().Allocate(type)));
    }

    Node& Node::AddInput(const std::string &name, PinType type) {
        return AddInput(new Pin(name, SR_SRLM_NS::DataTypeAllocator::Instance().Allocate(type)));
    }

    Node& Node::AddOutput(const std::string &name, PinType type) {
        return AddOutput(new Pin(name, SR_SRLM_NS::DataTypeAllocator::Instance().Allocate(type)));
    }

    Node& Node::AddInput(const std::string& name, SR_SRLM_NS::DataType* pDataType) {
        return AddInput(new Pin(name, pDataType));
    }

    Node& Node::AddOutput(const std::string& name, SR_SRLM_NS::DataType* pDataType) {
        return AddOutput(new Pin(name, pDataType));
    }*/

    uint64_t Node::GetHashName() const {
        //return m_logicalNode->GetNodeHashName();
        return 0;
    }

    SR_MATH_NS::FVector2 Node::GetPosition() const {
    #ifdef SR_USE_IMGUI_NODE_EDITOR
        auto&& pos = ax::NodeEditor::GetNodePosition(GetId());
        return SR_MATH_NS::FVector2(pos.x, pos.y);
    #else
        return SR_MATH_NS::FVector2();
    #endif
    }

    int32_t Node::GetPinIndex(const Pin* pPin) const {
        for (int32_t i = 0; i < m_inputs.size(); ++i) {
            if (m_inputs.at(i) == pPin) {
                return i;
            }
        }

        for (int32_t i = 0; i < m_outputs.size(); ++i) {
            if (m_outputs.at(i) == pPin) {
                return i;
            }
        }

        return SR_ID_INVALID;
    }

    void Node::RemoveInput(uint32_t index) {
        if (m_inputs.size() > index) {
            delete m_inputs.at(index);
            m_inputs.erase(m_inputs.begin() + index);
        }
    }

    void Node::RemoveOutput(uint32_t index) {
        if (m_outputs.size() > index) {
            delete m_outputs.at(index);
            m_outputs.erase(m_outputs.begin() + index);
        }
    }
}