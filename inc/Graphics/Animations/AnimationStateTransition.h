//
// Created by Monika on 23.04.2023.
//

#ifndef SR_ENGINE_ANIMATIONSTATETRANSITION_H
#define SR_ENGINE_ANIMATIONSTATETRANSITION_H

#include <Graphics/Animations/AnimationStateCondition.h>

namespace SR_ANIMATIONS_NS {
    class AnimationState;
    class AnimationStateMachine;

    class AnimationStateTransition : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<AnimationStateTransition> {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<AnimationStateTransition>;

    public:
        AnimationStateTransition();
        ~AnimationStateTransition() override;

    public:
        SR_NODISCARD virtual bool IsSuitable(const StateConditionContext& context) const noexcept;
        SR_NODISCARD virtual bool IsFinished(const StateConditionContext& context) const noexcept;

        void OnTransitionBegin(const StateConditionContext& context);

        SR_NODISCARD AnimationState* GetDestination() const noexcept { return m_destinationState; }
        SR_NODISCARD AnimationState* GetSource() const noexcept { return m_sourceState; }

        SR_NODISCARD bool IsActive() const noexcept { return m_isActive; }

        SR_NODISCARD float_t GetProgress() const noexcept { return m_condition ? m_condition->GetProgress().value_or(1.f) : 1.f; }

        void SetSourceState(AnimationState* state) { m_sourceState = state; }
        void SetDestinationState(AnimationState* state) { m_destinationState = state; }
        void SetTargetIndex(int32_t index) { m_targetIndex = index; }

        void ResetCondition() { m_condition.Reset(); }

        SR_NODISCARD int32_t GetTargetIndex() const noexcept { return m_targetIndex; }

        virtual void ResetTransition();
        virtual void Update(const StateConditionContext& context);

    protected:
        /// @property
        int32_t m_targetIndex = -1;
        /// @property
        AnimationStateCondition::Ptr m_condition;

        bool m_isActive = false;
        AnimationState* m_sourceState = nullptr;
        AnimationState* m_destinationState = nullptr;

    };
}

#endif //SR_ENGINE_ANIMATIONSTATETRANSITION_H
