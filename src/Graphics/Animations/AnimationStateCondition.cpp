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

    float_t AnimationTransitionExitTime::GetSourceProgress(const StateConditionContext& context) noexcept {
        if (!context.pState) {
            return 0.f;
        }
        /// стейты без длительности (например, точка входа) считаются проигранными сразу же
        if (context.pState->GetDuration() <= 0.f) {
            return 1.f;
        }
        return SR_CLAMP(context.pState->GetProgress(), 0.f, 1.f);
    }

    bool AnimationTransitionExitTime::IsSuitable(const StateConditionContext& context) const noexcept {
        return !m_hasExitTime || m_exitTimeReached;
    }

    bool AnimationTransitionExitTime::IsFinished(const StateConditionContext& context) const noexcept {
        /// переход еще не начинался
        if (m_dtDuration < 0.f) {
            return false;
        }
        return m_dtCapacity >= m_dtDuration;
    }

    float_t AnimationTransitionExitTime::GetProgress() const noexcept {
        /// переход еще не начинался - целевой стейт не должен вносить вклад
        if (m_dtDuration < 0.f) {
            return 0.f;
        }

        /// мгновенный переход без блендинга
        if (m_dtDuration <= 0.f) {
            return 1.f;
        }

        return SR_CLAMP(m_dtCapacity / m_dtDuration, 0.f, 1.f);
    }

    void AnimationTransitionExitTime::Reset() {
        m_dtCapacity = 0.f;
        m_dtDuration = -1.f;
        m_lastSourceProgress = -1.f;
        m_sourceLoops = 0;
        m_exitTimeReached = false;
    }

    void AnimationTransitionExitTime::Evaluate(const StateConditionContext& context) {
        if (!m_hasExitTime || m_exitTimeReached) {
            return;
        }

        /// у стейта нет длительности (например, точка входа) - ждать нечего
        if (!context.pState || context.pState->GetDuration() <= 0.f) {
            m_exitTimeReached = true;
            return;
        }

        if (m_exitTime <= 0.f) {
            m_exitTimeReached = true;
            return;
        }

        const float_t progress = GetSourceProgress(context);

        /// первый кадр наблюдения - запоминаем, с какого места играет стейт, и ждем пересечения exitTime
        if (m_lastSourceProgress < 0.f) SR_UNLIKELY_ATTRIBUTE {
            m_lastSourceProgress = progress;
            return;
        }

        /// прогресс откатился назад - значит стейт зациклился, то есть доиграл до конца
        const bool looped = progress + static_cast<float_t>(SR_EPSILON) < m_lastSourceProgress;
        if (looped) {
            ++m_sourceLoops;
        }

        if (m_exitTime > 1.f) {
            /// exitTime больше единицы - стейт должен проиграться несколько раз
            m_exitTimeReached = static_cast<float_t>(m_sourceLoops) + progress >= m_exitTime;
        }
        else {
            /// стейт должен именно пересечь exitTime, а не просто оказаться за ним в момент входа в стейт,
            /// иначе переход сработает мгновенно, если стейт был начат не с нуля
            m_exitTimeReached = m_lastSourceProgress < m_exitTime && (looped || progress >= m_exitTime);
        }

        m_lastSourceProgress = progress;
    }

    void AnimationTransitionExitTime::Update(const StateConditionContext& context) {
        /// длительность блендинга фиксируется в момент начала перехода
        if (m_dtDuration < 0.f) SR_UNLIKELY_ATTRIBUTE {
            const float_t stateDuration = context.pState ? context.pState->GetDuration() : 0.f;
            m_dtDuration = SR_MAX(stateDuration * m_duration, 0.f);
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
