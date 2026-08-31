//
// Created by Monika on 06.05.2023.
//

#include <Graphics/Animations/AnimationStateTransition.h>
#include <Graphics/Animations/AnimationState.h>

#include <Codegen/AnimationStateTransition.generated.hpp>

namespace SR_ANIMATIONS_NS {
    AnimationStateTransition::AnimationStateTransition()
        : Ptr(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    AnimationStateTransition::~AnimationStateTransition() {
        m_sourceState = nullptr;
        m_destinationState = nullptr;
    }

    bool AnimationStateTransition::IsSuitable(const StateConditionContext& context) const noexcept {
        if (m_condition && !m_condition->IsSuitable(context)) {
            return false;
        }

        //if (!m_exitTime.IsSuitable(context)) {
        //    return false;
        //}

        return true;
    }

    bool AnimationStateTransition::IsFinished(const StateConditionContext& context) const noexcept {
        if (!m_exitTime.IsFinished(context)) {
            return false;
        }

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

    void AnimationStateTransition::ResetTransition() {
        m_isActive = false;
        if (m_condition) {
            m_condition->ResetCondition();
        }
        m_exitTime.Reset();
    }

    void AnimationStateTransition::Evaluate(const StateConditionContext& context) {
        if (m_condition) {
            m_condition->Evaluate(context);
        }
    }

    void AnimationStateTransition::Update(const StateConditionContext& context) {
        m_exitTime.Update(context);
    }
}
