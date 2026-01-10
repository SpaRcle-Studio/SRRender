//
// Created by Monika on 06.05.2023.
//

#include <Graphics/Animations/AnimationGraphNode.h>
#include <Graphics/Animations/AnimationPose.h>
#include <Graphics/Animations/AnimationGraph.h>
#include <Graphics/Animations/AnimationStateMachine.h>

#include <Codegen/AnimationGraphNode.generated.hpp>

namespace SR_ANIMATIONS_NS {
    AnimationGraphNode::AnimationGraphNode(uint16_t input, uint16_t output)
        : SR_HTYPES_NS::SharedPtr<AnimationGraphNode>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
        , m_pose(new AnimationPose())
    {
        m_inputPins.resize(input);
        m_outputPins.resize(output);
    }

    AnimationGraphNode::~AnimationGraphNode() {
        SR_SAFE_DELETE_PTR(m_pose);
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

        if (m_inputPins.front().IsConnected()) {
            if (auto&& pNode = m_graph->GetNode(m_inputPins.front().m_targetNodeIndex)) {
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

    AnimationGraphNodeStateMachine::~AnimationGraphNodeStateMachine() = default;

    void AnimationGraphNodeStateMachine::SetStateMachine(AnimationStateMachine* pMachine) {
        m_stateMachine.AutoFree();
        m_stateMachine = pMachine;
        if (m_stateMachine) {
            m_stateMachine->SetNode(this);
        }
    }

    AnimationGraphNodeStateMachine::AnimationGraphNodeStateMachine()
        : Super(0, 1)
    {
        SetStateMachine(new AnimationStateMachine());
    }

    void AnimationGraphNodeStateMachine::OnPostLoad() {
        Super::OnPostLoad();
        if (m_stateMachine) {
            m_stateMachine->SetNode(this);
        }
    }

    void AnimationGraphNodeStateMachine::CloneTo(SR_UTILS_NS::SRClass& clone) const {
        Super::CloneTo(clone);
        if (auto&& pStateMachine = static_cast<AnimationGraphNodeStateMachine&>(clone).m_stateMachine) {
            pStateMachine->SetNode(static_cast<AnimationGraphNodeStateMachine*>(&clone));
        }
    }
}
