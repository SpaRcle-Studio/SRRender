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

    void AnimationStateConditionAnd::Update(const StateConditionContext& context) {
        Super::Update(context);

        if (m_conditions.empty()) {
            return;
        }

        for (auto&& pCondition : m_conditions) {
            pCondition->Update(context);

            if (pCondition->IsNeedBreakUpdate()) {
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

    bool AnimationStateConditionAnd::IsNeedBreakUpdate() const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (pCondition->IsNeedBreakUpdate()) {
                return true;
            }
        }

        return false;
    }

    std::optional<float_t> AnimationStateConditionAnd::GetProgress() const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (auto&& progress = pCondition->GetProgress()) {
                return progress;
            }
        }
        return Super::GetProgress();
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

    bool AnimationStateConditionOr::IsNeedBreakUpdate() const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (pCondition->IsNeedBreakUpdate()) {
                return true;
            }
        }

        return false;
    }

    std::optional<float_t> AnimationStateConditionOr::GetProgress() const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (auto&& progress = pCondition->GetProgress()) {
                return progress;
            }
        }
        return Super::GetProgress();
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

    void AnimationStateConditionNot::Update(const StateConditionContext &context) {
        if (m_condition) {
            m_condition->Update(context);
        }
        Super::Update(context);
    }

    void AnimationStateConditionNot::ResetCondition() {
        if (m_condition) {
            m_condition->ResetCondition();
        }
        Super::ResetCondition();
    }

    AnimationStateConditionNot::~AnimationStateConditionNot() = default;

    bool AnimationStateConditionNot::IsNeedBreakUpdate() const noexcept {
        return m_condition && m_condition->IsNeedBreakUpdate();
    }

    std::optional<float_t> AnimationStateConditionNot::GetProgress() const noexcept {
        return m_condition ? m_condition->GetProgress() : Super::GetProgress();
    }

    /// ----------------------------------------------------------------------------------------------------------------

    bool AnimationStateConditionExitTime::IsSuitable(const StateConditionContext& context) const noexcept {
        if (!context.pState) {
            return false;
        }

        if (m_hasExitTime) {
            return m_dtCapacity >= m_dtExitTime;
        }

        return true;
    }

    bool AnimationStateConditionExitTime::IsFinished(const StateConditionContext& context) const noexcept {
        if (!context.pState) {
            return false;
        }

        if (m_hasExitTime) {
            return (m_dtCapacity - m_dtExitTime) >= m_dtDuration;
        }
        return m_dtCapacity >= m_dtDuration;
    }

    std::optional<float_t> AnimationStateConditionExitTime::GetProgress() const noexcept {
        if (m_dtDuration <= 0.f) {
            return 1.f;
        }

        if (m_hasExitTime) {
            const float_t progress = (m_dtCapacity - m_dtExitTime) / m_dtDuration;
            return SR_MIN(progress, 1.f);
        }

        const float_t progress = m_dtCapacity / m_dtDuration;
        return SR_MIN(progress, 1.f);
    }

    void AnimationStateConditionExitTime::ResetCondition() {
        m_dtDuration = 0.f;
        m_dtCapacity = 0.f;
        Super::ResetCondition();
    }

    void AnimationStateConditionExitTime::Update(const StateConditionContext& context) {
        if (m_hasExitTime && m_dtExitTime <= 0.f) SR_UNLIKELY_ATTRIBUTE {
            m_dtExitTime = context.pState ? (context.pState->GetDuration() * m_exitTime) : 0.f;
        }

        if (m_dtDuration <= 0.f) SR_UNLIKELY_ATTRIBUTE {
            m_dtDuration = context.pState ? (context.pState->GetDuration() * m_duration) : 0.f;
        }

        m_dtCapacity += context.dt;

        Super::Update(context);
    }

    /// ----------------------------------------------------------------------------------------------------------------

    bool AnimationStateConditionBool::IsSuitable(const StateConditionContext& context) const noexcept {
        return m_checked;
    }

    void AnimationStateConditionBool::Update(const StateConditionContext& context) {
        if (!m_checked) {
            auto&& value = context.pMachine->GetBool(m_variableName);
            m_checked = value.has_value() && m_value == value.value();
        }
        Super::Update(context);
    }

    void AnimationStateConditionBool::ResetCondition() {
        m_checked = false;
        Super::ResetCondition();
    }
}
