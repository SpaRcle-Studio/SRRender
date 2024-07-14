//
// Created by Monika on 08.05.2023.
//

#include <Graphics/Animations/AnimationStateMachine.h>

namespace SR_ANIMATIONS_NS {
    AnimationStateMachine::AnimationStateMachine()
        : Super()
    {
        CreateState<AnimationEntryPointState>();
        m_activeStates.insert(GetEntryPoint());
    }

    AnimationStateMachine::~AnimationStateMachine() {
        for (auto&& pState : m_states) {
            delete pState;
        }
    }

    AnimationStateMachine* AnimationStateMachine::Load(const SR_XML_NS::Node& nodeXml) {
        SR_TRACY_ZONE;

        auto&& pStateMachine = new AnimationStateMachine();

        for (auto&& stateXml : nodeXml.GetNodes("State")) {
            auto&& type = stateXml.GetAttribute("Type").ToString();

            if (auto&& pState = AnimationState::Load(stateXml)) {
                pStateMachine->AddState(pState);
            }
            else {
                SR_ERROR("AnimationStateMachine::Load() : failed to load state \"{}\"!", type);
                pStateMachine->AddState(new AnimationNoneState());
            }
        }

        for (auto&& transitionXml : nodeXml.GetNodes("Transition")) {
            auto&& from = transitionXml.GetAttribute("From").ToUInt();
            auto&& to = transitionXml.GetAttribute("To").ToUInt();

            if (from == to) {
                SR_ERROR("AnimationStateMachine::Load() : cycle transition detected! \"{}\"", from);
                continue;
            }

            auto&& pFromState = pStateMachine->GetState(from);
            if (!pFromState) {
                SR_ERROR("AnimationStateMachine::Load() : state \"{}\" not found!", from);
                continue;
            }

            auto&& pToState = pStateMachine->GetState(to);
            if (!pToState) {
                SR_ERROR("AnimationStateMachine::Load() : state \"{}\" not found!", to);
                continue;
            }

            auto&& pTransition = AnimationStateTransition::Load(pFromState, pToState, transitionXml);
            if (!pTransition) {
                SR_ERROR("AnimationStateMachine::Load() : failed to load transition!");
                continue;
            }

            pFromState->AddTransition(pTransition);
        }

        return pStateMachine;
    }

    void AnimationStateMachine::Update(UpdateContext& context) {
        SR_TRACY_ZONE;

        StateConditionContext stateConditionContext;
        stateConditionContext.pMachine = this;
        stateConditionContext.dt = context.dt;

        for (auto pIt = m_activeStates.begin(); pIt != m_activeStates.end(); ) {
            AnimationState* pState = *pIt;

            stateConditionContext.pState = pState;

            bool changed = false;
            bool hasActiveTransitions = false;

            for (auto&& pTransition : pState->GetTransitions()) {
                pTransition->Update(stateConditionContext);

                if (!pTransition->IsSuitable(stateConditionContext)) {
                    continue;
                }

                auto&& pDestinationState = pTransition->GetDestination();
                if (!pDestinationState) {
                    continue;
                }

                if (!pTransition->IsActive()) {
                    pTransition->OnTransitionBegin(stateConditionContext);
                }

                hasActiveTransitions = true;

                const float_t progress = pTransition->GetProgress();

                if (progress < 0.f || progress > 1.f) {
                    SRHaltOnce("AnimationStateMachine::Update() : invalid progress \"{}\"!", progress);
                    continue;
                }

                //SR_DEBUG_LOG("Transition \"{}\" -> \"{}\" : progress \"{}\"", pState->GetName().c_str(), pDestinationState->GetName().c_str(), progress);

                UpdateContext transitionFromContext = context;
                if (1.f - progress > 0.f) {
                    transitionFromContext.weight = 1.f - progress;
                    pState->Update(transitionFromContext);
                }

                if (progress > 0.f) {
                    UpdateContext transitionToContext = context;
                    transitionToContext.weight = progress;
                    pDestinationState->Update(transitionToContext);
                }

                if (pTransition->IsFinished(stateConditionContext)) {
                    pState->OnTransitionDone();

                    if (m_activeStates.count(pState) == 1) {
                        pIt = m_activeStates.erase(pIt);
                    }

                    pIt = m_activeStates.insert(pIt, pDestinationState);

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

    void AnimationStateMachine::Compile(CompileContext& context) {
        for (auto&& pState : m_states) {
            pState->Compile(context);
        }
    }

    AnimationEntryPointState* AnimationStateMachine::GetEntryPoint() const {
        if (m_states.empty()) {
            SRHalt("Entry point not exists!");
            return nullptr;
        }

        if (auto&& pState = dynamic_cast<AnimationEntryPointState*>(m_states.front())) {
            return pState;
        }

        SRHalt("Failed to get entry point!");

        return nullptr;
    }

    AnimationState* AnimationStateMachine::FindState(SR_UTILS_NS::StringAtom name) const {
        for (auto&& pState : m_states) {
            if (pState->GetName() == name) {
                return pState;
            }
        }

        return nullptr;
    }

    AnimationState* AnimationStateMachine::GetState(uint32_t index) const {
        if (index >= m_states.size()) {
            SRHalt("Index out of range!");
            return nullptr;
        }

        return m_states[index];
    }
}
