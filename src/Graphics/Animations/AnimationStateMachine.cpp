//
// Created by Monika on 08.05.2023.
//

#include <Graphics/Animations/AnimationStateMachine.h>

#include <Codegen/AnimationStateMachine.generated.hpp>

namespace SR_ANIMATIONS_NS {
    AnimationStateMachine::AnimationStateMachine()
        : Super()
    {
        m_states.emplace_back(SRNew<AnimationEntryPointState>());
    }

    AnimationStateMachine::~AnimationStateMachine() {
        m_states.clear();
    }

    void AnimationStateMachine::OnPostLoad() {
        Super::OnPostLoad();

        if (m_states.empty()) {
            SR_ERROR("AnimationStateMachine::OnPostLoad() : no entry point state found! Trying to fix...");
            m_states.emplace_back(SRNew<AnimationEntryPointState>());
        }

        for (auto&& pState : m_states) {
            pState->SetMachine(this);
        }
    }

    void AnimationStateMachine::CloneTo(SR_UTILS_NS::SRClass& clone) const {
        Super::CloneTo(clone);
        for (auto&& pState : static_cast<AnimationStateMachine&>(clone).m_states) {
            pState->SetMachine(&static_cast<AnimationStateMachine&>(clone));
        }
    }

    void AnimationStateMachine::Update(UpdateContext& context) {
        SR_TRACY_ZONE;

        const uint32_t maxTransitions = 64;
        uint32_t transitionCount = 0;

        for (auto pIt = m_activeStates.begin(); pIt != m_activeStates.end(); ) {
            if (transitionCount >= maxTransitions) {
                SR_WARN("AnimationStateMachine::Update() : max transitions count \"{}\" reached!", maxTransitions);
                break;
            }

            AnimationState* pState = *pIt;
            if (!pState) {
                SRHalt("Invalid state in active states!");
                continue;
            }

            bool changed = false;
            bool hasActiveTransitions = false;

            for (auto&& pTransition : pState->GetTransitions()) {
                auto&& pActiveTransition = pState->GetActiveTransition();
                if (pActiveTransition && pActiveTransition != pTransition.Get()) {
                    continue;
                }

                if (UpdateTransition(context, pTransition.Get(), hasActiveTransitions)) {
                    if (m_activeStates.count(pTransition->GetSource()) == 1) {
                        pIt = m_activeStates.erase(pIt);
                    }
                    pIt = m_activeStates.insert(pIt, pTransition->GetDestination());
                    transitionCount++;
                    changed = true;
                }
            }

            if (!hasActiveTransitions) {
                pState->Update(context);
            }

            if (!changed) {
                ++pIt;
            }
        }
    }

    void AnimationStateMachine::ForEachState(const SR_HTYPES_NS::Function<void(AnimationState&)>& callback) {
        for (auto&& pState : m_states) {
            if (!pState) {
                continue;
            }
            callback(*pState);
        }
    }

    bool AnimationStateMachine::SetSimpleClip(const SR_HTYPES_NS::SharedPtr<AnimationClip>& pClip) {
        SR_TRACY_ZONE;

        m_states.resize(2);

        if (!m_states[0]) {
            m_states[0] = SRNew<AnimationEntryPointState>();
            m_states[0]->SetMachine(this);
        }

        if (!m_states[1] || m_states[1]->GetMeta() != AnimationClipState::GetMetaStatic()) {
            m_states[1] = SRNew<AnimationClipState>();
            m_states[1]->SetMachine(this);
        }

        m_states[1]->GetTransitions().clear();

        auto&& pEntryPoint = m_states[0].StaticCast<AnimationEntryPointState>();
        pEntryPoint->GetTransitions().resize(1);
        auto&& pTransition = pEntryPoint->GetTransitions()[0];
        if (!pTransition) {
            pTransition = SRNew<AnimationStateTransition>();
        }

        pTransition->ResetCondition();
        pTransition->SetTargetIndex(1);

        return m_states[1].StaticCast<AnimationClipState>()->SetClip(pClip);
    }

    bool AnimationStateMachine::RemoveState(AnimationState* pState) {
        if (!pState) {
            return false;
        }

        for (uint32_t i = 0; i < m_states.size(); ++i) {
            if (m_states[i] == pState) {
                return RemoveState(i);
            }
        }

        return false;
    }

    bool AnimationStateMachine::RemoveState(const uint32_t index) {
        if (index >= m_states.size()) {
            return false;
        }

        // не даём удалить entry point
        if (index == 0) {
            return false;
        }

        // удаляем транзишены на удаляемый стейт
        for (auto&& pState : m_states) {
            if (!pState) {
                continue;
            }
            pState->OnStateRemoved(index);
        }

        m_states.erase(m_states.begin() + static_cast<int64_t>(index));

        // правим индексы переходов
        for (auto&& pState : m_states) {
            if (!pState) {
                continue;
            }

            for (auto&& pTransition : pState->GetTransitions()) {
                if (!pTransition) {
                    continue;
                }

                const int32_t target = pTransition->GetTargetIndex();
                if (target < 0) {
                    continue;
                }

                if (static_cast<uint32_t>(target) == index) {
                    pTransition->SetTargetIndex(-1);
                }
                else if (static_cast<uint32_t>(target) > index) {
                    pTransition->SetTargetIndex(target - 1);
                }
            }
        }

        // форсим пересбор активных стейтов при следующем Compile()
        m_activeStates.clear();

        return true;
    }

    void AnimationStateMachine::Compile(CompileContext& context) {
        for (auto&& pState : m_states) {
            pState->Compile(context);
        }

        if (m_activeStates.empty()) {
            if (auto&& pEntryPoint = GetEntryPoint()) {
                m_activeStates.insert(pEntryPoint);
            }
            else {
                SR_WARN("AnimationStateMachine::Compile() : entry point state not found!");
            }
        }
    }

    bool AnimationStateMachine::IsStateActive(SR_UTILS_NS::StringAtom name) const {
        SR_TRACY_ZONE;

        for (auto&& pState : m_activeStates) {
            if (pState->GetStateName() == name) {
                return true;
            }

            for (auto&& pTransition : pState->GetTransitions()) {
                auto&& pActiveTransition = pState->GetActiveTransition();
                if (pActiveTransition && pActiveTransition != pTransition.Get()) {
                    continue;
                }

                StateConditionContext stateConditionContext;
                stateConditionContext.pMachine = this;
                stateConditionContext.pState = pTransition->GetSource();

                if (!pTransition->IsSuitable(stateConditionContext)) {
                    return false;
                }

                if (pTransition->GetDestination() && pTransition->GetDestination()->GetStateName() == name) {
                    return true;
                }
            }
        }

        return false;
    }

    AnimationEntryPointState* AnimationStateMachine::GetEntryPoint() const {
        if (m_states.empty()) {
            SRHalt("Entry point not exists!");
            return nullptr;
        }

        if (auto&& pState = dynamic_cast<const AnimationEntryPointState*>(m_states.front().Get())) {
            return const_cast<AnimationEntryPointState*>(pState);
        }

        SRHalt("Failed to get entry point!");

        return nullptr;
    }

    AnimationState* AnimationStateMachine::FindState(SR_UTILS_NS::StringAtom name) const {
        for (auto&& pState : m_states) {
            if (pState->GetStateName() == name) {
                return const_cast<AnimationState*>(pState.Get());
            }
        }

        return nullptr;
    }

    AnimationState* AnimationStateMachine::GetState(uint32_t index) const {
        if (auto&& pState = GetStateOrNull(index)) {
            return pState;
        }
        SRHalt("AnimationStateMachine::GetState() : state index \"{}\" out of bounds!", index);
        return nullptr;
    }

    AnimationState* AnimationStateMachine::GetStateOrNull(uint32_t index) const {
        return index < m_states.size() ? const_cast<AnimationState*>(m_states[index].Get()) : nullptr;
    }

    bool AnimationStateMachine::UpdateTransition(UpdateContext& context, AnimationStateTransition* pTransition, bool&  hasActiveTransitions) {
        StateConditionContext stateConditionContext;
        stateConditionContext.pMachine = this;
        stateConditionContext.dt = context.dt;
        stateConditionContext.pState = pTransition->GetSource();

        pTransition->Update(stateConditionContext);

        if (!pTransition->IsSuitable(stateConditionContext)) {
            return false;
        }

        auto&& pDestinationState = pTransition->GetDestination();
        if (!pDestinationState) {
            return false;
        }

        if (!pTransition->IsActive()) {
            pTransition->OnTransitionBegin(stateConditionContext);
        }

        hasActiveTransitions = true;

        const float_t progress = pTransition->GetProgress();

        if (progress < 0.f || progress > 1.f) {
            SRHaltOnce("AnimationStateMachine::Update() : invalid progress \"{}\"!", progress);
            return false;
        }

        UpdateContext transitionFromContext = context;
        if (1.f - progress > 0.f) {
            transitionFromContext.weight = 1.f - progress;
            pTransition->GetSource()->Update(transitionFromContext);
        }

        if (progress > 0.f) {
            UpdateContext transitionToContext = context;
            transitionToContext.weight = progress;
            pDestinationState->Update(transitionToContext);
        }

        if (pTransition->IsFinished(stateConditionContext)) {
            pTransition->GetDestination()->OnTransitionDone();
            return true;
        }

        return false;
    }

    void AnimationStateMachine::FastForwardState(AnimationState* pState) {
        if (!pState) {
            return;
        }

        pState->ResetState();
        m_activeStates.clear();
        m_activeStates.insert(pState);
    }
}
