//
// Created by Monika on 06.05.2023.
//

#include <Graphics/Animations/AnimationGraphNode.h>
#include <Graphics/Animations/AnimationPose.h>

namespace SR_ANIMATIONS_NS {
    AnimationGraphNode::AnimationGraphNode(uint16_t input, uint16_t output)
        : m_pose(new AnimationPose())
    {
        m_inputPins.resize(input);
        m_outputPins.resize(output);
    }

    AnimationGraphNode::~AnimationGraphNode() {
        SR_SAFE_DELETE_PTR(m_pose);
    }

    AnimationGraphNode* AnimationGraphNode::Load(const SR_XML_NS::Node& nodeXml) {
        SR_TRACY_ZONE;

        auto&& type = nodeXml.GetAttribute("Type").ToString();
        if (type == "StateMachine") {
            return AnimationGraphNodeStateMachine::Load(nodeXml);
        }

        SR_ERROR("AnimationGraphNode::Load() : unknown type \"{}\"!", type);

        return nullptr;
    }

    bool AnimationGraphNode::IsStateActive(SR_UTILS_NS::StringAtom name) const {
        return false;
    }

    uint64_t AnimationGraphNode::GetIndex() const {
        return m_graph->GetNodeIndex(this);
    }

    void AnimationGraphNode::ConnectTo(AnimationGraphNode* pNode, uint16_t fromPinIndex, uint16_t toPinIndex) {
        if (!pNode) {
            SRHalt("Invalid node!");
            return;
        }

        if (fromPinIndex >= m_outputPins.size()) {
            SRHalt("Out of range!");
            return;
        }

        if (toPinIndex >= pNode->m_inputPins.size()) {
            SRHalt("Out of range!");
            return;
        }

        m_outputPins[fromPinIndex] = AnimationLink(pNode->GetIndex(), toPinIndex);
        pNode->m_inputPins[toPinIndex] = AnimationLink(GetIndex(), fromPinIndex);
    }

    /// ----------------------------------------------------------------------------------------------------------------

    AnimationPose* AnimationGraphNodeFinal::Update(UpdateContext& context, const AnimationLink& from) {
        SR_TRACY_ZONE;

        if (m_inputPins.front().has_value()) {
            if (auto&& pNode = m_graph->GetNode(m_inputPins.front().value().m_targetNodeIndex)) {
                return pNode->Update(context, AnimationLink(0, 0));
            }
        }

        return nullptr;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    AnimationPose* AnimationGraphNodeStateMachine::Update(UpdateContext& context, const AnimationLink& from) {
        SR_TRACY_ZONE;

        if (m_stateMachine) {
            context.pPose = m_pose;
            m_stateMachine->Update(context);
        }

        return m_pose;
    }

    void AnimationGraphNodeStateMachine::Compile(CompileContext& context) {
        SR_TRACY_ZONE;

        if (m_stateMachine) {
            m_stateMachine->SetAnimationDataSetParent(m_graph);
            m_stateMachine->Compile(context);
        }

        m_pose->SetGameObjectsCount(context.gameObjects.size());

        Super::Compile(context);
    }

    bool AnimationGraphNodeStateMachine::IsStateActive(SR_UTILS_NS::StringAtom name) const {
        SR_TRACY_ZONE;

        if (m_stateMachine) {
            return m_stateMachine->IsStateActive(name);
        }

        return false;
    }

    AnimationGraphNodeStateMachine::~AnimationGraphNodeStateMachine() {
        SR_SAFE_DELETE_PTR(m_stateMachine);
    }

    AnimationGraphNodeStateMachine* AnimationGraphNodeStateMachine::Load(const SR_XML_NS::Node& nodeXml) {
        auto&& pStateMachine = AnimationStateMachine::Load(nodeXml);
        if (!pStateMachine) {
            SR_ERROR("AnimationGraphNodeStateMachine::Load() : failed to load state machine!");
            return nullptr;
        }

        auto&& pNode = new AnimationGraphNodeStateMachine();
        pNode->SetStateMachine(pStateMachine);
        return pNode;
    }

    void AnimationGraphNodeStateMachine::SetStateMachine(AnimationStateMachine* pMachine) {
        SR_SAFE_DELETE_PTR(m_stateMachine);
        m_stateMachine = pMachine;
        if (m_stateMachine) {
            m_stateMachine->SetNode(this);
        }
    }
}
