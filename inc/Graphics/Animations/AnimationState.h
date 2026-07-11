//
// Created by Monika on 08.05.2023.
//

#ifndef SR_ENGINE_ANIMATIONSTATE_H
#define SR_ENGINE_ANIMATIONSTATE_H

#include <Graphics/Animations/AnimationStateTransition.h>

#include <Utils/Math/Vector2.h>

namespace SR_ANIMATIONS_NS {
    class AnimationGraph;
    class AnimationClip;
    class AnimationStateMachine;

    /// @abstract
    class AnimationState : public SR_HTYPES_NS::SharedPtr<AnimationState>, public SR_UTILS_NS::Serializable {
        SR_CLASS()
        using Transitions = std::vector<AnimationStateTransition::Ptr>;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<AnimationState>;

    public:
        AnimationState();
        ~AnimationState() override;

    public:
        SR_NODISCARD AnimationStateMachine* GetMachine() const noexcept { return m_machine; }
        SR_NODISCARD SR_MATH_NS::FVector2 GetEditorPosition() const noexcept { return m_editorPosition; }
        void SetEditorPosition(const SR_MATH_NS::FVector2& pos) noexcept { m_editorPosition = pos; }

        SR_NODISCARD virtual float_t GetProgress() const noexcept { return 1.f; }
        SR_NODISCARD virtual float_t GetDuration() const noexcept { return 0.f; }
        SR_NODISCARD virtual float_t GetTime() const noexcept { return 0.f; }

        SR_NODISCARD AnimationStateTransition* GetActiveTransition() const noexcept { return m_activeTransition; }

        SR_NODISCARD Transitions& GetTransitions() noexcept { return m_transitions; }
        SR_NODISCARD const Transitions& GetTransitions() const noexcept { return m_transitions; }
        SR_NODISCARD bool IsResetOnPlay() const noexcept { return m_resetOnPlay; }
        SR_NODISCARD uint32_t GetStateIndex() const noexcept;

        void OnTransitionBegin(AnimationStateTransition* pTransition);
        void OnTransitionDone();
        virtual void Update(const UpdateContext& context) { }

        void SetMachine(AnimationStateMachine* pMachine) { m_machine = pMachine; }
        void SetResetOnPlay(bool reset) { m_resetOnPlay = reset; }

        void OnStateRemoved(uint32_t stateIndex);

        virtual void ResetState() { }

        SR_NODISCARD virtual SR_UTILS_NS::StringAtom GetDefaultStateName() const noexcept;
        SR_NODISCARD SR_UTILS_NS::StringAtom GetStateName() const noexcept;

        virtual void Update(UpdateContext& context) { }
        virtual bool Compile(CompileContext& context);

        void SetUserData(void* pUserData) { m_userData = pUserData; }
        SR_NODISCARD void* GetUserData() const noexcept { return m_userData; }
        template<typename T> SR_NODISCARD T* GetUserData() const noexcept { return reinterpret_cast<T*>(m_userData); }

    protected:
        /// @property @getter(GetStateName)
        SR_UTILS_NS::StringAtom m_stateName;
        /// @property @group(Editor) @hidden
        SR_MATH_NS::FVector2 m_editorPosition = SR_MATH_NS::FVector2(0.f, 0.f);
        /// @property
        bool m_resetOnPlay = false;
        /// @property @notNull @hidden
        Transitions m_transitions;

        AnimationStateTransition* m_activeTransition = nullptr;
        AnimationStateMachine* m_machine = nullptr;
        void* m_userData = nullptr;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationNoneState : public AnimationState {
        SR_CLASS()
        using Super = AnimationState;
    public:

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationClipState : public AnimationState {
        SR_CLASS()
        using Super = AnimationState;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<AnimationClipState>;

    public:
        ~AnimationClipState() override;

        void OnPostLoad() override;
        void CloneTo(SR_UTILS_NS::SRClass& clone) const override;

        void Update(UpdateContext& context) override;
        bool Compile(CompileContext& context) override;
        void ResetState() override;
        bool SetClip(const SR_HTYPES_NS::SharedPtr<AnimationClip>& pClip);

        SR_NODISCARD float_t GetProgress() const noexcept override;
        SR_NODISCARD float_t GetDuration() const noexcept override { return m_duration; }
        SR_NODISCARD float_t GetTime() const noexcept override { return m_time; }

        SR_NODISCARD SR_UTILS_NS::StringAtom GetDefaultStateName() const noexcept override;

    private:
        void UpdateClip();

    protected:
        /// @property
        /// @customArgs(pick: enabled, filter name: Animation, relative: resources)
        /// @customArg(filter value: animation)
        SR_UTILS_NS::Path m_animation;

        std::vector<ChannelUpdateContext> m_channelContexts;
        SR_HTYPES_NS::SharedPtr<AnimationClip> m_clip;
        uint32_t m_maxKeyFrame = 0;
        float_t m_duration = 0.f;
        float_t m_time = 0.f;
        SR_HTYPES_NS::FastMemoryArray<uint32_t> m_channelPlayState;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    /// @hidden
    class AnimationEntryPointState : public AnimationState {
        SR_CLASS()
        using Super = AnimationState;
    public:

    };
}

#endif //SR_ENGINE_ANIMATIONSTATE_H
