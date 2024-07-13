//
// Created by Monika on 06.05.2023.
//

#ifndef SR_ENGINE_ANIMATIONSTATECONDITION_H
#define SR_ENGINE_ANIMATIONSTATECONDITION_H

#include <Utils/Types/Time.h>
#include <Graphics/Animations/AnimationContext.h>

namespace SR_ANIMATIONS_NS {
    class AnimationStateTransition;

    class AnimationStateCondition : public SR_UTILS_NS::NonCopyable {
    public:
        using Super = SR_UTILS_NS::NonCopyable;
        using Hash = uint64_t;
    public:
        SR_NODISCARD static AnimationStateCondition* Load(const SR_XML_NS::Node& nodeXml);

        SR_NODISCARD virtual bool IsSuitable(const StateConditionContext& context) const noexcept = 0;
        SR_NODISCARD virtual bool IsFinished(const StateConditionContext& context) const noexcept {
            return IsSuitable(context);
        }

        SR_NODISCARD virtual float_t GetProgress() const noexcept { return 1.f; }

        virtual void Reset() { }
        virtual void Update(const StateConditionContext& context) { }

        void SetTransitionState(AnimationStateTransition* pTransition) { m_transitionState = pTransition; }

    protected:
        AnimationStateTransition* m_transitionState = nullptr;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationStateConditionTrue : public AnimationStateCondition {
        using Super = AnimationStateCondition;
    public:
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override { return true; }

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationStateConditionAnd : public AnimationStateCondition {
        using Super = AnimationStateCondition;
    public:
        ~AnimationStateConditionAnd() override;

    public:
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsFinished(const StateConditionContext& context) const noexcept override;

        void Reset() override;

    protected:
        std::vector<AnimationStateCondition*> m_conditions;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationStateConditionOr : public AnimationStateCondition {
        using Super = AnimationStateCondition;
    public:
        ~AnimationStateConditionOr() override;

    public:
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsFinished(const StateConditionContext& context) const noexcept override;

        void Reset() override;

    protected:
        std::vector<AnimationStateCondition*> m_conditions;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationStateConditionNot : public AnimationStateCondition {
        using Super = AnimationStateCondition;
    public:
        ~AnimationStateConditionNot() override;

    public:
        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsFinished(const StateConditionContext& context) const noexcept override;

        void Reset() override;

    protected:
        AnimationStateCondition* m_condition = nullptr;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationStateConditionExitTime : public AnimationStateCondition {
        using Super = AnimationStateCondition;
    public:
        SR_NODISCARD static AnimationStateConditionExitTime* Load(const SR_XML_NS::Node& nodeXml);

        SR_NODISCARD bool IsSuitable(const StateConditionContext& context) const noexcept override;
        SR_NODISCARD bool IsFinished(const StateConditionContext& context) const noexcept override;

        SR_NODISCARD float_t GetProgress() const noexcept override;

        void Reset() override;
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
        float_t m_duration = 0.25f;
        float_t m_exitTime = 0.75f;
        bool m_hasExitTime = true;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    //class AnimationStateConditionOperationBase : public AnimationStateCondition {
    //    AnimationStateConditionOperationType m_type = AnimationStateConditionOperationType::Equals;

    //};

    /// ----------------------------------------------------------------------------------------------------------------

    //class AnimationStateConditionBool : public AnimationStateConditionOperationBase {
    //private:
    //    struct BoolVariable {
    //        bool m_useConst = true;
    //        bool m_const = false;
    //        Hash m_variable = 0;
    //    } m_data;
    //};
}

#endif //SR_ENGINE_ANIMATIONSTATECONDITION_H
