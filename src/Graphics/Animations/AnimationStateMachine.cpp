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
            auto&& type = nodeXml.GetAttribute("Type").ToString();

            if (auto&& pState = AnimationState::Load(stateXml)) {
                pStateMachine->AddState(pState);
            }
            else {
                SR_ERROR("AnimationStateMachine::Load() : failed to load state \"{}\"!", type);
            }
        }

        for (auto&& transitionXml : nodeXml.GetNodes("Transition")) {
            auto&& from = transitionXml.GetAttribute("From").ToString();
            auto&& to = transitionXml.GetAttribute("To").ToString();
            if (from.empty() || to.empty()) {
                SR_ERROR("AnimationStateMachine::Load() : invalid transition!");
                continue;
            }

            if (from == to) {
                SR_ERROR("AnimationStateMachine::Load() : cycle transition detected! \"{}\"", from);
                continue;
            }

            auto&& pFromState = pStateMachine->FindState(from);
            if (!pFromState) {
                SR_ERROR("AnimationStateMachine::Load() : state \"{}\" not found!", from);
                continue;
            }

            auto&& pToState = pStateMachine->FindState(to);
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

        for (auto pIt = m_activeStates.begin(); pIt != m_activeStates.end(); ) {
            AnimationState* pState = *pIt;

            pState->Update(context);

            StateConditionContext stateConditionContext;
            stateConditionContext.pState = pState;
            stateConditionContext.pMachine = this;

            bool changed = false;

            for (auto&& pTransition : pState->GetTransitions()) {
                if (!pTransition->IsSuitable(stateConditionContext)) {
                    continue;
                }

                auto&& pDestinationState = pTransition->GetDestination();
                if (!pDestinationState) {
                    continue;
                }

                pDestinationState->OnTransitionBegin(context);

                if (pTransition->IsFinished(stateConditionContext)) {
                    pDestinationState->OnTransitionEnd(context);

                    if (m_activeStates.count(pState) == 1) {
                        pIt = m_activeStates.erase(pIt);
                    }

                    pIt = m_activeStates.insert(pIt, pDestinationState);

                    changed = true;
                }
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
}
