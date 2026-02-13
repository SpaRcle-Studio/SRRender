//
// Created by Monika on 06.05.2023.
//

#ifndef SR_ENGINE_ANIMATIONSTATECONDITION_H
#define SR_ENGINE_ANIMATIONSTATECONDITION_H

#include <Graphics/Animations/AnimationContext.h>

namespace SR_ANIMATIONS_NS {
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

        SR_NODISCARD virtual bool IsNeedBreakUpdate() const noexcept { return false; }

        SR_NODISCARD virtual std::optional<float_t> GetProgress() const noexcept { return std::nullopt; }

        virtual void ResetCondition() { }
        virtual void Update(const StateConditionContext& context) { }

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
        SR_NODISCARD bool IsNeedBreakUpdate() const noexcept override;
        SR_NODISCARD std::optional<float_t> GetProgress() const noexcept override;

        void ResetCondition() override;
        void Update(const StateConditionContext& context) override;

    protected:
        /// @property @notNull
        std::vector<AnimationStateCondition::Ptr> m_conditions;

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
        SR_NODISCARD bool IsNeedBreakUpdate() const noexcept override;
        SR_NODISCARD std::optional<float_t> GetProgress() const noexcept override;

        void ResetCondition() override;

    protected:
        /// @property @notNull
        std::vector<AnimationStateCondition::Ptr> m_conditions;

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
        SR_NODISCARD bool IsNeedBreakUpdate() const noexcept override;
        SR_NODISCARD std::optional<float_t> GetProgress() const noexcept override;

        void Update(const StateConditionContext& context) override;
        void ResetCondition() override;

    protected:
        /// @property @notNull
        AnimationStateCondition::Ptr m_condition;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationStateConditionExitTime : public AnimationStateCondition {
        SR_CLASS()
        using Super = AnimationStateCondition;
    public:
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsFinished(const StateConditionContext& context) const noexcept override;

        SR_NODISCARD std::optional<float_t> GetProgress() const noexcept override;

        void ResetCondition() override;
        void Update(const StateConditionContext& context) override;

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
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override;
        void Update(const StateConditionContext& context) override;
        SR_NODISCARD bool IsNeedBreakUpdate() const noexcept override { return !m_checked; }

        void ResetCondition() override;

    private:
        /// @property
        SR_UTILS_NS::StringAtom m_variableName;
        /// @property
        bool m_value = false;

        bool m_checked = false;

    };
}

#endif //SR_ENGINE_ANIMATIONSTATECONDITION_H
