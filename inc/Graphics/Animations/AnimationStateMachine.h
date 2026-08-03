//
// Created by Monika on 23.04.2023.
//

#ifndef SR_ENGINE_ANIMATIONSTATEMACHINE_H
#define SR_ENGINE_ANIMATIONSTATEMACHINE_H

#include <Graphics/Animations/AnimationState.h>

namespace SR_ANIMATIONS_NS {
    class AnimationClip;
    class AnimationGraph;

    class AnimationStateMachine : public IAnimationDataSet {
        SR_CLASS()
        using Super = IAnimationDataSet;
    public:
        AnimationStateMachine();
        ~AnimationStateMachine() override;

    public:
        void OnPostLoad() override;
        void CloneTo(SR_UTILS_NS::SRClass& clone) const override;

        SR_NODISCARD bool IsStateActive(SR_UTILS_NS::StringAtom name) const;
        SR_NODISCARD AnimationEntryPointState* GetEntryPoint() const;
        SR_NODISCARD AnimationState* FindState(SR_UTILS_NS::StringAtom name) const;
        SR_NODISCARD AnimationState* GetState(uint32_t index) const;
        SR_NODISCARD AnimationState* GetStateOrNull(uint32_t index) const;
        SR_NODISCARD const SR_UTILS_NS::Vector<AnimationState::Ptr>& GetStates() const noexcept { return m_states; }
        SR_NODISCARD SR_UTILS_NS::Vector<AnimationState::Ptr>& GetStatesMutable() noexcept { return m_states; }

        void ForEachState(const SR_HTYPES_NS::Function<void(AnimationState&)>& callback);
        void FastForwardState(AnimationState* pState);

        bool SetSimpleClip(const SR_HTYPES_NS::SharedPtr<AnimationClip>& pClip);
        void Compile(CompileContext& context);
        void Update(UpdateContext& context);

        bool RemoveState(uint32_t index);
        bool RemoveState(AnimationState* pState);

        template<class T, typename... Args> T* CreateState(Args&& ...args) {
            return AddState(new T(std::forward<Args>(args)...));
        }

        template<class T> T* AddState(T* pState) {
            SR_STATIC_ASSERT2((std::is_base_of_v<AnimationState, T>), "T must be derived from AnimationState");
            m_states.emplace_back(dynamic_cast<AnimationState*>(pState));
            pState->SetMachine(this);
            return pState;
        }

        void SetNode(AnimationGraphNode* pNode) { m_node = pNode; }

    private:
        bool UpdateTransition(UpdateContext& context, AnimationStateTransition* pTransition, bool& hasActiveTransitions);

    private:
        AnimationGraphNode* m_node = nullptr;

        /// @property @hidden
        SR_UTILS_NS::Vector<AnimationState::Ptr> m_states;

        SR_UTILS_NS::Set<AnimationState*> m_activeStates;

    };
}

#endif //SR_ENGINE_ANIMATIONSTATEMACHINE_H
