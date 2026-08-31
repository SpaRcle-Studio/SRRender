//
// Created by Monika on 06.05.2023.
//

#include <Graphics/Animations/AnimationStateCondition.h>
#include <Graphics/Animations/AnimationState.h>
#include <Graphics/Animations/AnimationStateMachine.h>

#include <Codegen/AnimationStateCondition.generated.hpp>

namespace SR_ANIMATIONS_NS {
    AnimationStateCondition::AnimationStateCondition()
        : Ptr(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    bool AnimationStateConditionAnd::IsSuitable(const StateConditionContext& context) const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (!pCondition->IsSuitable(context)) {
                return false;
            }
        }

        return true;
    }

    void AnimationStateConditionAnd::ResetCondition() {
        for (auto&& pCondition : m_conditions) {
            pCondition->ResetCondition();
        }
        Super::ResetCondition();
    }

    void AnimationStateConditionAnd::Evaluate(const StateConditionContext& context) {
        Super::Evaluate(context);

        if (m_conditions.empty()) {
            return;
        }

        for (auto&& pCondition : m_conditions) {
            pCondition->Evaluate(context);
            if (pCondition->IsNeedBreakEvaluation()) {
                break;
            }
        }
    }

    /// ----------------------------------------------------------------------------------------------------------------

    AnimationStateConditionAnd::~AnimationStateConditionAnd() = default;

    bool AnimationStateConditionAnd::IsFinished(const StateConditionContext& context) const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (!pCondition->IsFinished(context)) {
                return false;
            }
        }

        return true;
    }

    bool AnimationStateConditionAnd::IsNeedBreakEvaluation() const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (pCondition->IsNeedBreakEvaluation()) {
                return true;
            }
        }

        return false;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    AnimationStateConditionOr::~AnimationStateConditionOr() = default;

    bool AnimationStateConditionOr::IsSuitable(const StateConditionContext& context) const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (pCondition->IsSuitable(context)) {
                return true;
            }
        }

        return false;
    }

    void AnimationStateConditionOr::ResetCondition() {
        for (auto&& pCondition : m_conditions) {
            pCondition->ResetCondition();
        }
        Super::ResetCondition();
    }

    bool AnimationStateConditionOr::IsFinished(const StateConditionContext &context) const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (pCondition->IsFinished(context)) {
                return true;
            }
        }

        return false;
    }

    bool AnimationStateConditionOr::IsNeedBreakEvaluation() const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (pCondition->IsNeedBreakEvaluation()) {
                return true;
            }
        }

        return false;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    bool AnimationStateConditionNot::IsSuitable(const StateConditionContext& context) const noexcept {
        if (!m_condition) {
            return false;
        }
        return !m_condition->IsSuitable(context);
    }

    bool AnimationStateConditionNot::IsFinished(const StateConditionContext& context) const noexcept {
        if (!m_condition) {
            return false;
        }
        return !m_condition->IsFinished(context);
    }

    void AnimationStateConditionNot::Evaluate(const StateConditionContext &context) {
        if (m_condition) {
            m_condition->Evaluate(context);
        }
        Super::Evaluate(context);
    }

    void AnimationStateConditionNot::ResetCondition() {
        if (m_condition) {
            m_condition->ResetCondition();
        }
        Super::ResetCondition();
    }

    AnimationStateConditionNot::~AnimationStateConditionNot() = default;

    bool AnimationStateConditionNot::IsNeedBreakEvaluation() const noexcept {
        return m_condition && m_condition->IsNeedBreakEvaluation();
    }

    /// ----------------------------------------------------------------------------------------------------------------

    bool AnimationTransitionExitTime::IsSuitable(const StateConditionContext& context) const noexcept {
        if (!context.pState) {
            return false;
        }

        if (m_hasExitTime) {
            return m_dtCapacity >= m_dtExitTime;
        }

        return true;
    }

    bool AnimationTransitionExitTime::IsFinished(const StateConditionContext& context) const noexcept {
        if (!context.pState) {
            return false;
        }

        if (m_hasExitTime) {
            return (m_dtCapacity - m_dtExitTime) >= m_dtDuration;
        }
        return m_dtCapacity >= m_dtDuration;
    }

    float_t AnimationTransitionExitTime::GetProgress() const noexcept {
        if (m_dtDuration <= 0.f) {
            return 1.f;
        }

        if (m_hasExitTime) {
            const float_t progress = (m_dtCapacity - m_dtExitTime) / m_dtDuration;
            return SR_CLAMP(progress, 0.f, 1.f);
        }

        const float_t progress = m_dtCapacity / m_dtDuration;
        return SR_CLAMP(progress, 0.f, 1.f);
    }

    void AnimationTransitionExitTime::Reset() {
        m_dtDuration = 0.f;
        m_dtCapacity = 0.f;
        m_dtExitTime = 0.f;
    }

    void AnimationTransitionExitTime::Update(const StateConditionContext& context) {
        if (m_hasExitTime && m_dtExitTime <= 0.f) SR_UNLIKELY_ATTRIBUTE {
            m_dtExitTime = context.pState ? (context.pState->GetDuration() * m_exitTime) : 0.f;
        }

        if (m_dtDuration <= 0.f) SR_UNLIKELY_ATTRIBUTE {
            m_dtDuration = context.pState ? (context.pState->GetDuration() * m_duration) : 0.f;
        }

        m_dtCapacity += context.dt;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    bool AnimationStateConditionBool::IsSuitable(const StateConditionContext& context) const noexcept {
        return m_checked;
    }

    void AnimationStateConditionBool::Evaluate(const StateConditionContext& context) {
        if (!m_checked) {
            auto&& value = context.pMachine->GetBool(m_variableName);
            m_checked = value.has_value() && m_value == value.value();
        }
        Super::Evaluate(context);
    }

    void AnimationStateConditionBool::ResetCondition() {
        m_checked = false;
        Super::ResetCondition();
    }

    void AnimationStateConditionFloat::Evaluate(const StateConditionContext& context) {
        if (!m_checked) {
            auto&& value = context.pMachine->GetFloat(m_variableName);
            if (!value) {
                m_checked = false;
            }
            else {
                switch (m_comparison) {
                    case CompareType::Equal:
                        m_checked = SR_MATH_NS::IsEquals(value.value(), m_value);
                        break;
                    case CompareType::NotEqual:
                        m_checked = !SR_MATH_NS::IsEquals(value.value(), m_value);
                        break;
                    case CompareType::Less:
                        m_checked = value.value() < m_value;
                        break;
                    case CompareType::LessOrEqual:
                        m_checked = value.value() <= m_value;
                        break;
                    case CompareType::Greater:
                        m_checked = value.value() > m_value;
                        break;
                    case CompareType::GreaterOrEqual:
                        m_checked = value.value() >= m_value;
                        break;
                    default:
                        SRHalt("AnimationStateConditionFloat::Evaluate() : unknown comparison type!");
                        m_checked = false;
                        break;
                }
            }
        }
        Super::Evaluate(context);
    }

    bool AnimationStateConditionFloat::IsSuitable(const StateConditionContext& context) const noexcept {
        return m_checked;
    }

    void AnimationStateConditionFloat::ResetCondition() {
        m_checked = false;
        Super::ResetCondition();
    }
}
