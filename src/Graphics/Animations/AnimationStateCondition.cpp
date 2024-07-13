//
// Created by Monika on 06.05.2023.
//

#include <Graphics/Animations/AnimationStateCondition.h>

namespace SR_ANIMATIONS_NS {
    bool AnimationStateConditionAnd::IsSuitable(const StateConditionContext& context) const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (!pCondition->IsSuitable(context)) {
                return false;
            }
        }

        return false;
    }

    void AnimationStateConditionAnd::Reset() {
        for (auto&& pCondition : m_conditions) {
            pCondition->Reset();
        }
        Super::Reset();
    }

    AnimationStateCondition* AnimationStateCondition::Load(const SR_XML_NS::Node& nodeXml) {
        SR_TRACY_ZONE;

        auto&& type = nodeXml.GetAttribute("Type").ToString();
        if (type == "True") {
            return new AnimationStateConditionTrue();
        }

        /*if (type == "And") {
            auto&& pCondition = new AnimationStateConditionAnd();
            for (auto&& node : nodeXml.GetChildren()) {
                if (auto&& pChild = Load(node)) {
                    pCondition->m_conditions.push_back(pChild);
                }
            }

            return pCondition;
        }

        if (type == "Or") {
            auto&& pCondition = new AnimationStateConditionOr();
            for (auto&& node : nodeXml.GetChildren()) {
                if (auto&& pChild = Load(node)) {
                    pCondition->m_conditions.push_back(pChild);
                }
            }

            return pCondition;
        }

        if (type == "Not") {
            auto&& pCondition = new AnimationStateConditionNot();
            for (auto&& node : nodeXml.GetChildren()) {
                if (auto&& pChild = Load(node)) {
                    pCondition->m_condition = pChild;
                }
            }

            return pCondition;
        }*/

        if (type == "ExitTime") {
            return AnimationStateConditionExitTime::Load(nodeXml);
        }

        SR_ERROR("AnimationStateCondition::Load() : unknown type \"{}\"!", type);

        return nullptr;
    }

    AnimationStateConditionAnd::~AnimationStateConditionAnd() {
        for (auto&& pCondition : m_conditions) {
            delete pCondition;
        }
    }

    bool AnimationStateConditionAnd::IsFinished(const StateConditionContext &context) const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (!pCondition->IsFinished(context)) {
                return false;
            }
        }

        return true;
    }

    /// ----------------------------------------------------------------------------------------------------------------

    AnimationStateConditionOr::~AnimationStateConditionOr() {
        for (auto&& pCondition : m_conditions) {
            delete pCondition;
        }
    }

    bool AnimationStateConditionOr::IsSuitable(const StateConditionContext& context) const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (pCondition->IsSuitable(context)) {
                return true;
            }
        }

        return false;
    }

    void AnimationStateConditionOr::Reset() {
        for (auto&& pCondition : m_conditions) {
            pCondition->Reset();
        }
        Super::Reset();
    }

    bool AnimationStateConditionOr::IsFinished(const StateConditionContext &context) const noexcept {
        for (auto&& pCondition : m_conditions) {
            if (pCondition->IsFinished(context)) {
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

    void AnimationStateConditionNot::Reset() {
        if (m_condition) {
            m_condition->Reset();
        }

        Super::Reset();
    }

    AnimationStateConditionNot::~AnimationStateConditionNot() {
        SR_SAFE_DELETE_PTR(m_condition);
    }

    /// ----------------------------------------------------------------------------------------------------------------

    AnimationStateConditionExitTime* AnimationStateConditionExitTime::Load(const SR_XML_NS::Node& nodeXml) {
        SR_TRACY_ZONE;

        auto&& pCondition = new AnimationStateConditionExitTime();

        pCondition->m_exitTime = nodeXml.GetAttribute("ExitTime").ToFloat();
        pCondition->m_hasExitTime = nodeXml.GetAttribute("HasExitTime").ToBool();
        pCondition->m_duration = nodeXml.GetAttribute("Duration").ToFloat();

        return pCondition;
    }

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

        return (m_dtCapacity - m_dtExitTime) >= m_dtDuration;
    }

    float_t AnimationStateConditionExitTime::GetProgress() const noexcept {
        if (m_dtDuration <= 0.f) {
            return 1.f;
        }

        return (m_dtCapacity - m_dtExitTime) / m_dtDuration;
    }

    void AnimationStateConditionExitTime::Reset() {
        m_dtDuration = 0.f;
        m_dtCapacity = 0.f;
        Super::Reset();
    }

    void AnimationStateConditionExitTime::Update(const StateConditionContext& context) {
        if (m_hasExitTime && m_dtExitTime <= 0.f) SR_UNLIKELY_ATTRIBUTE {
            m_dtExitTime = context.pState->GetDuration() * m_exitTime;
        }

        if (m_dtDuration <= 0.f) SR_UNLIKELY_ATTRIBUTE {
            m_dtDuration = context.pState->GetDuration() * m_duration;
        }

        m_dtCapacity += context.dt;

        Super::Update(context);
    }
}
