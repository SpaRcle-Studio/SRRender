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

        return true;
    }

    void AnimationStateConditionAnd::Reset() {
        for (auto&& pCondition : m_conditions) {
            pCondition->Reset();
        }
        Super::Reset();
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

    AnimationStateCondition* AnimationStateCondition::Load(const SR_XML_NS::Node& nodeXml) {
        SR_TRACY_ZONE;

        auto&& type = nodeXml.GetAttribute("Type").ToString();
        if (type == "True") {
            return new AnimationStateConditionTrue();
        }

        if (type == "Bool") {
            return AnimationStateConditionBool::Load(nodeXml);
        }

        if (type == "And") {
            return AnimationStateConditionAnd::Load(nodeXml);
        }

        /*if (type == "Or") {
            auto&& pCondition = new AnimationStateConditionOr();
            for (auto&& node : nodeXml.GetChildren()) {
                if (auto&& pChild = Load(node)) {
                    pCondition->m_conditions.push_back(pChild);
                }
            }

            return pCondition;
        }*/

        if (type == "Not") {
            return AnimationStateConditionNot::Load(nodeXml);
        }

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

    AnimationStateConditionAnd* AnimationStateConditionAnd::Load(const SR_XML_NS::Node& nodeXml) {
        auto&& pCondition = new AnimationStateConditionAnd();
        for (auto&& childNodeXml : nodeXml.GetNodes("Condition")) {
            if (auto&& pChild = AnimationStateCondition::Load(childNodeXml)) {
                pCondition->m_conditions.push_back(pChild);
            }
        }

        return pCondition;
    }

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

    void AnimationStateConditionNot::Reset() {
        if (m_condition) {
            m_condition->Reset();
        }
        Super::Reset();
    }

    AnimationStateConditionNot::~AnimationStateConditionNot() {
        SR_SAFE_DELETE_PTR(m_condition);
    }

    AnimationStateConditionNot* AnimationStateConditionNot::Load(const SR_XML_NS::Node& nodeXml) {
        auto&& pCondition = new AnimationStateConditionNot();
        if (auto&& xmlCondition = nodeXml.GetNode("Condition")) {
            pCondition->m_condition = AnimationStateCondition::Load(xmlCondition);
        }

        return pCondition;
    }

    bool AnimationStateConditionNot::IsNeedBreakUpdate() const noexcept {
        return m_condition && m_condition->IsNeedBreakUpdate();
    }

    std::optional<float_t> AnimationStateConditionNot::GetProgress() const noexcept {
        return m_condition ? m_condition->GetProgress() : Super::GetProgress();
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

    /// ----------------------------------------------------------------------------------------------------------------

    AnimationStateConditionBool* AnimationStateConditionBool::Load(const SR_XML_NS::Node& nodeXml) {
        auto&& pCondition = new AnimationStateConditionBool();
        pCondition->m_variableName = nodeXml.GetAttribute("Variable").ToString();
        pCondition->m_value = nodeXml.GetAttribute("Value").ToBool();
        return pCondition;
    }

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

    void AnimationStateConditionBool::Reset() {
        m_checked = false;
        Super::Reset();
    }
}
