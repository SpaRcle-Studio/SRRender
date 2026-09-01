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

    /**
        Работает по аналогии с Unity.

        exitTime измеряется в нормализованном времени исходного стейта (0..1, значения больше 1
        означают проигрывание нескольких циклов). Переход не может начаться, пока исходный стейт
        не проиграется до exitTime. Если hasExitTime = false, то переход начнется сразу же,
        как только выполнятся условия.

        duration - длительность блендинга, задается в долях от длительности исходного стейта.
        Отсчитывается с момента фактического начала перехода. Если duration = 0, то переход
        происходит мгновенно, без блендинга.
    */
    class AnimationTransitionExitTime : public SR_UTILS_NS::Serializable {
        SR_CLASS()
    public:
        /// Достигнуто ли время выхода, то есть можно ли начинать переход.
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept;
        /// Отработал ли блендинг уже начатого перехода.
        SR_NODISCARD bool IsFinished(const StateConditionContext& context) const noexcept;

        /// Прогресс блендинга (0 - полностью исходный стейт, 1 - полностью целевой).
        SR_NODISCARD float_t GetProgress() const noexcept;

        SR_NODISCARD bool HasExitTime() const noexcept { return m_hasExitTime; }

        void Reset();

        /// Вызывается каждый кадр, пока переход еще не начался. Следит за проигрыванием исходного стейта.
        void Evaluate(const StateConditionContext& context);
        /// Вызывается каждый кадр, пока переход активен. Накапливает время блендинга.
        void Update(const StateConditionContext& context);

    private:
        SR_NODISCARD static float_t GetSourceProgress(const StateConditionContext& context) noexcept;

    protected:
        /// Накопленное время блендинга в секундах.
        float_t m_dtCapacity = 0.f;
        /// Длительность блендинга в секундах. Вычисляется в момент начала перехода, -1 - еще не вычислена.
        float_t m_dtDuration = -1.f;
        /// Прогресс исходного стейта на предыдущем кадре. Нужен для отлова пересечения exitTime
        /// и зацикливания стейта. Отрицательное значение - стейт еще ни разу не опрашивался.
        float_t m_lastSourceProgress = -1.f;
        /// Сколько раз исходный стейт успел зациклиться с момента сброса перехода.
        uint32_t m_sourceLoops = 0;
        /// Исходный стейт доиграл до exitTime.
        bool m_exitTimeReached = false;

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
