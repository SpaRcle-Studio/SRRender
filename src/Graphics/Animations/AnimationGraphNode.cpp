//
// Created by Monika on 06.05.2023.
//

#include <Graphics/Animations/AnimationGraphNode.h>
#include <Graphics/Animations/AnimationPose.h>
#include <Graphics/Animations/AnimationGraph.h>
#include <Graphics/Animations/AnimationStateMachine.h>

#include <Codegen/AnimationGraphNode.generated.hpp>

namespace SR_ANIMATIONS_NS {
    AnimationGraphNode::AnimationGraphNode()
        : AnimationGraphNode(0, 0)
    { }

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

    AnimationLink* AnimationGraphNode::GetInputPin(SR_UTILS_NS::StringView name) {
        for (auto&& pin : m_inputPins) {
            if (pin.name == name) {
                return &pin;
            }
        }

        return nullptr;
    }

    AnimationLink* AnimationGraphNode::GetOutputPin(SR_UTILS_NS::StringView name) {
        for (auto&& pin : m_outputPins) {
            if (pin.name == name) {
                return &pin;
            }
        }

        return nullptr;
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

        m_outputPins[fromPinIndex].Connect(pNode->GetIndex(), toPinIndex);
        pNode->m_inputPins[toPinIndex].Connect(GetIndex(), fromPinIndex);
    }

    void AnimationGraphNode::Compile(CompileContext& context) {
        if (SRVerify(m_pose)) {
            m_pose->SetGameObjectsCount(context.gameObjects.size());
        }
    }

    void AnimationGraphNode::OnNodeRemoved(AnimationGraphNode* pNode) {
        for (auto&& pin : m_inputPins) {
            if (pin.m_targetNodeIndex == pNode->GetIndex()) {
                pin.Disconnect();
            }
        }

        for (auto&& pin : m_outputPins) {
            if (pin.m_targetNodeIndex == pNode->GetIndex()) {
                pin.Disconnect();
            }
        }

        const auto index = m_graph->GetNodeIndex(pNode);
        for (auto&& pin : m_inputPins) {
            if (pin.m_targetNodeIndex > index) {
                --pin.m_targetNodeIndex;
            }
        }
        for (auto&& pin : m_outputPins) {
            if (pin.m_targetNodeIndex > index) {
                --pin.m_targetNodeIndex;
            }
        }
    }

    void AnimationGraphNode::CloneTo(SR_UTILS_NS::SRClass& clone) const {
        Serializable::CloneTo(clone);
        auto&& target = static_cast<AnimationGraphNode&>(clone);
        for (size_t i = 0; i < m_inputPins.size(); ++i) {
            target.m_inputPins[i].name = m_inputPins[i].name;
        }
        for (size_t i = 0; i < m_outputPins.size(); ++i) {
            target.m_outputPins[i].name = m_outputPins[i].name;
        }
    }

    void AnimationGraphNode::BreakLink(uint32_t inputPinIndex) {
        if (inputPinIndex >= m_inputPins.size()) {
            SRHalt("Out of range!");
            return;
        }

        auto&& link = m_inputPins[inputPinIndex];
        if (!link.IsConnected()) {
            return;
        }

        if (auto&& pNode = m_graph->GetNode(link.m_targetNodeIndex)) {
            if (link.m_targetPinIndex < pNode->m_outputPins.size()) {
                pNode->m_outputPins[link.m_targetPinIndex].Disconnect();
            }
        }

        m_graph->InvalidateCompile();
        link.Disconnect();
    }

    /// ----------------------------------------------------------------------------------------------------------------

    void AnimationGraphNodeFinal::OnPostLoad() {
        Super::OnPostLoad();
        m_inputPins[0].name = "Pose";
    }

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

    bool AnimationGraphNodeStateMachine::SetSimpleClip(const SR_HTYPES_NS::SharedPtr<AnimationClip>& pClip) {
        SR_TRACY_ZONE;

        if (!m_stateMachine) {
            SetStateMachine(new AnimationStateMachine());
        }

        return m_stateMachine->SetSimpleClip(pClip);
    }

    void AnimationGraphNodeStateMachine::Compile(CompileContext& context) {
        SR_TRACY_ZONE;

        if (m_stateMachine) {
            m_stateMachine->SetAnimationDataSetParent(m_graph);
            m_stateMachine->Compile(context);
        }

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
        m_outputPins[0].name = "Pose";
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

    /// ----------------------------------------------------------------------------------------------------------------

    AnimationGraphNodeExternalPose::AnimationGraphNodeExternalPose()
        : Super(0, 1)
    { }

    void AnimationGraphNodeExternalPose::OnPostLoad() {
        m_outputPins[0].name = "Pose";
        Super::OnPostLoad();
    }

    AnimationPose* AnimationGraphNodeExternalPose::Update(UpdateContext& context, const AnimationLink& from) {
        return m_pose;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    AnimationGraphNodeLinearBlend::AnimationGraphNodeLinearBlend()
        : Super(2, 1)
    { }

    void AnimationGraphNodeLinearBlend::OnPostLoad() {
        m_inputPins[0].name = "Pose A";
        m_inputPins[1].name = "Pose B";
        m_outputPins[0].name = "Pose";
        Super::OnPostLoad();
    }

    void AnimationGraphNodeLinearBlend::Compile(CompileContext& context) {
        SR_TRACY_ZONE;
        Super::Compile(context);
    }

    AnimationPose* AnimationGraphNodeLinearBlend::Update(UpdateContext& context, const AnimationLink& from) {
        SR_TRACY_ZONE;

        if (SR_MATH_NS::IsEquals(m_blendFactor, 0.f)) {
            if (m_inputPins.front().IsConnected()) {
                if (auto&& pNode = m_graph->GetNode(m_inputPins.front().m_targetNodeIndex)) {
                    return pNode->Update(context, AnimationLink(0, 0));
                }
            }

            return nullptr;
        }
        else if (SR_MATH_NS::IsEquals(m_blendFactor, 1.f)) {
            if (m_inputPins.size() > 1 && m_inputPins[1].IsConnected()) {
                if (auto&& pNode = m_graph->GetNode(m_inputPins[1].m_targetNodeIndex)) {
                    return pNode->Update(context, AnimationLink(0, 0));
                }
            }

            return nullptr;
        }

        for (size_t i = 0; i < m_poses.size(); ++i) {
            auto& inputPin = m_inputPins[i];
            if (inputPin.IsConnected()) {
                if (auto&& pNode = m_graph->GetNode(inputPin.m_targetNodeIndex)) {
                    m_poses[i] = pNode->Update(context, AnimationLink(0, 0));
                }
            }
        }

        if (m_poses[0] && m_poses[1]) {
            m_pose->BlendLinear(*m_poses[0], *m_poses[1], m_blendFactor, m_blendMode);
            return m_pose;
        }

        return nullptr;
    }
}
