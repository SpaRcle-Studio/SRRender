//
// Created by Monika on 11.01.2022.
//

#include <Graphics/GUI/Node.h>
#include <Graphics/GUI/NodeManager.h>
#include <Graphics/GUI/Pin.h>
#include <Graphics/GUI/NodeBuilder.h>

namespace SR_GRAPH_GUI_NS {
    Node::Node()
    { }

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

    uint64_t Node::GetHashName() const {
        return 0;
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