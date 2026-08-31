//
// Created by Monika on 06.05.2023.
//

#ifndef SR_ENGINE_ANIMATIONSTATECONDITION_H
#define SR_ENGINE_ANIMATIONSTATECONDITION_H

#include <Graphics/Animations/AnimationContext.h>

namespace SR_ANIMATIONS_NS {
    SR_ENUM_NS_CLASS_T(CompareType, uint8_t,
        Equal,
        NotEqual,
        Less,
        LessOrEqual,
        Greater,
        GreaterOrEqual
    );

    class AnimationStateTransition;

    /// @abstract
    class AnimationStateCondition : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<AnimationStateCondition> {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<AnimationStateCondition>;

    public:
        AnimationStateCondition();

    public:
        SR_NODISCARD virtual bool IsSuitable(const StateConditionContext& context) const noexcept { return false; }

        SR_NODISCARD virtual bool IsFinished(const StateConditionContext& context) const noexcept {
            return IsSuitable(context);
        }

        SR_NODISCARD virtual bool IsNeedBreakEvaluation() const noexcept { return false; }

        virtual void ResetCondition() { }
        virtual void Evaluate(const StateConditionContext& context) { }

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationStateConditionTrue : public AnimationStateCondition {
        SR_CLASS()
        using Super = AnimationStateCondition;
    public:
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override { return true; }

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationStateConditionAnd : public AnimationStateCondition {
        SR_CLASS()
        using Super = AnimationStateCondition;
    public:
        ~AnimationStateConditionAnd() override;

    public:
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsFinished(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsNeedBreakEvaluation() const noexcept override;

        void ResetCondition() override;
        void Evaluate(const StateConditionContext& context) override;

    protected:
        /// @property @notNull
        SR_UTILS_NS::Vector<AnimationStateCondition::Ptr> m_conditions;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationStateConditionOr : public AnimationStateCondition {
        SR_CLASS()
        using Super = AnimationStateCondition;
    public:
        ~AnimationStateConditionOr() override;

    public:
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsFinished(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsNeedBreakEvaluation() const noexcept override;

        void ResetCondition() override;

    protected:
        /// @property @notNull
        SR_UTILS_NS::Vector<AnimationStateCondition::Ptr> m_conditions;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationStateConditionNot : public AnimationStateCondition {
        SR_CLASS()
        using Super = AnimationStateCondition;
    public:
        ~AnimationStateConditionNot() override;

    public:
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsFinished(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsNeedBreakEvaluation() const noexcept override;

        void Evaluate(const StateConditionContext& context) override;
        void ResetCondition() override;

    protected:
        /// @property @notNull
        AnimationStateCondition::Ptr m_condition;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationTransitionExitTime : public SR_UTILS_NS::Serializable {
        SR_CLASS()
    public:
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept;
        SR_NODISCARD bool IsFinished(const StateConditionContext& context) const noexcept;

        SR_NODISCARD float_t GetProgress() const noexcept;

        void Reset();
        void Update(const StateConditionContext& context);

    protected:
        float_t m_dtCapacity = 0.f;
        float_t m_dtDuration = 0.f;
        float_t m_dtExitTime = 0.f;

        /**
            Измеряется в отношении времени относительно состоянияни из которого переходим.
            Если exitTime = 0.75, то переход начнется через 75% времени состояния.
            А если hasExitTime = false, то переход начнется сразу.
            duration - время за которое происходит переход.
            Если duration больше чем 1 - exitTime, то стейт начнется сначала.
        */

        /// @property
        float_t m_duration = 0.25f;
        /// @property
        float_t m_exitTime = 0.75f;
        /// @property
        bool m_hasExitTime = true;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationStateConditionBool : public AnimationStateCondition {
        SR_CLASS()
        using Super = AnimationStateCondition;
    public:
        void Evaluate(const StateConditionContext& context) override;
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsNeedBreakEvaluation() const noexcept override { return !m_checked; }

        void ResetCondition() override;

    private:
        /// @property
        SR_UTILS_NS::StringAtom m_variableName;
        /// @property
        bool m_value = false;

        bool m_checked = false;

    };

    class AnimationStateConditionFloat : public AnimationStateCondition {
        SR_CLASS()
        using Super = AnimationStateCondition;
    public:
        void Evaluate(const StateConditionContext& context) override;
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsNeedBreakEvaluation() const noexcept override { return !m_checked; }

        void ResetCondition() override;

    private:
        /// @property
        SR_UTILS_NS::StringAtom m_variableName;
        /// @property
        float_t m_value = 0.f;
        /// @property
        CompareType m_comparison = CompareType::Equal;

        bool m_checked = false;

    };
}

#endif //SR_ENGINE_ANIMATIONSTATECONDITION_H
