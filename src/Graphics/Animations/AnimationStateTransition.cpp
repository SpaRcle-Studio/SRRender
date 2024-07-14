//
// Created by Monika on 06.05.2023.
//

#include <Graphics/Animations/AnimationStateTransition.h>

namespace SR_ANIMATIONS_NS {
    AnimationStateTransition::AnimationStateTransition(AnimationState* pSource, AnimationState* pDestination, AnimationStateCondition* pCondition)
        : Super()
        , m_condition(pCondition)
        , m_sourceState(pSource)
        , m_destinationState(pDestination)
    { 
        SRAssert(m_destinationState != m_sourceState);
    }

    AnimationStateTransition::AnimationStateTransition(AnimationState* pSource, AnimationState* pDestination)
        : Super()
        , m_sourceState(pSource)
        , m_destinationState(pDestination)
    { 
        SRAssert(m_destinationState != m_sourceState);
    }

    AnimationStateTransition::~AnimationStateTransition() {
        SR_SAFE_DELETE_PTR(m_condition);
        m_sourceState = nullptr;
        m_destinationState = nullptr;
    }

    AnimationStateTransition* AnimationStateTransition::Load(AnimationState* pSource, AnimationState* pDestination, const SR_XML_NS::Node& nodeXml) {
        if (auto&& xmlCondition = nodeXml.GetNode("Condition")) {
            if (auto&& pCondition = AnimationStateCondition::Load(xmlCondition)) {
                return new AnimationStateTransition(pSource, pDestination, pCondition);
            }
        }

        return new AnimationStateTransition(pSource, pDestination);
    }

    bool AnimationStateTransition::IsSuitable(const StateConditionContext& context) const noexcept {
        if (m_condition) {
            return m_condition->IsSuitable(context);
        }

        return true;
    }

    bool AnimationStateTransition::IsFinished(const StateConditionContext& context) const noexcept {
        if (m_condition) {
            return m_condition->IsFinished(context);
        }

        return true;
    }

    void AnimationStateTransition::OnTransitionBegin(const StateConditionContext& context) {
        m_isActive = true;
        if (m_destinationState) {
            m_destinationState->OnTransitionBegin(this);
        }
    }

    void AnimationStateTransition::Reset() {
        m_isActive = false;
        if (m_condition) {
            m_condition->Reset();
        }
    }

    void AnimationStateTransition::Update(const StateConditionContext& context) {
        if (m_condition) {
            m_condition->Update(context);
        }
    }
}
