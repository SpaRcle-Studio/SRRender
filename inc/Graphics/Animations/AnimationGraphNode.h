//
// Created by Monika on 06.05.2023.
//

#ifndef SR_ENGINE_ANIMATIONGRAPHNODE_H
#define SR_ENGINE_ANIMATIONGRAPHNODE_H

#include <Graphics/Animations/AnimationCommon.h>
#include <Graphics/Animations/AnimationContext.h>

#include <Utils/Math/Vector2.h>

namespace SR_ANIMATIONS_NS {
    class AnimationStateMachine;
    class AnimationGraph;
    class AnimationPose;
    class AnimationClip;

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationGraphNode : public SR_UTILS_NS::Serializable, public SR_HTYPES_NS::SharedPtr<AnimationGraphNode> {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<AnimationGraphNode>;

    public:
        /**
         * @param input - сколько данная нода имеет входных пинов
         * @param output - сколько данная нода имеет выходных пинов
         */
        AnimationGraphNode();
        explicit AnimationGraphNode(uint16_t input, uint16_t output);
        ~AnimationGraphNode() override;

    public:
        SR_NODISCARD uint32_t GetInputCount() const noexcept { return static_cast<uint32_t>(m_inputPins.size()); }
        SR_NODISCARD uint32_t GetOutputCount() const noexcept { return static_cast<uint32_t>(m_outputPins.size()); }
        SR_NODISCARD const std::vector<AnimationLink>& GetInputLinks() const noexcept { return m_inputPins; }
        SR_NODISCARD const std::vector<AnimationLink>& GetOutputLinks() const noexcept { return m_outputPins; }
        SR_NODISCARD SR_MATH_NS::FVector2 GetEditorPosition() const noexcept { return m_editorPosition; }
        void SetEditorPosition(const SR_MATH_NS::FVector2& pos) noexcept { m_editorPosition = pos; }
        SR_NODISCARD virtual AnimationPose* Update(UpdateContext& context, const AnimationLink& from) { return nullptr; }
        SR_NODISCARD virtual bool IsStateActive(SR_UTILS_NS::StringAtom name) const;
		virtual void Compile(CompileContext& context);
        void SetGraph(AnimationGraph* pGraph) { m_graph = pGraph; }
        void AddInputPin(const AnimationLink& link) { m_inputPins.emplace_back(link); }
        void AddOutputPin(const AnimationLink& link) { m_outputPins.emplace_back(link); }
        void ClearInputPins() { m_inputPins.clear(); }
        void ClearOutputPins() { m_outputPins.clear(); }

        SR_NODISCARD uint64_t GetIndex() const;

        void ConnectTo(AnimationGraphNode* pNode, uint16_t fromPinIndex, uint16_t toPinIndex);

    protected:
        AnimationGraph* m_graph = nullptr;
        AnimationPose* m_pose = nullptr;

        /// @property @hidden
        std::vector<AnimationLink> m_inputPins;

        /// @property @group(Editor) @hidden
        SR_MATH_NS::FVector2 m_editorPosition = SR_MATH_NS::FVector2(0.f, 0.f);

        std::vector<AnimationLink> m_outputPins;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationGraphNodeFinal : public AnimationGraphNode {
        SR_CLASS()
        using Super = AnimationGraphNode;
    public:
        explicit AnimationGraphNodeFinal()
            : Super(1, 0)
        { }

    public:
        SR_NODISCARD AnimationPose* Update(UpdateContext& context, const AnimationLink& from) override;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationGraphNodeStateMachine : public AnimationGraphNode {
        SR_CLASS()
        using Super = AnimationGraphNode;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<AnimationGraphNodeStateMachine>;

    public:
        AnimationGraphNodeStateMachine();
        ~AnimationGraphNodeStateMachine() override;

    public:
        void OnPostLoad() override;
        void CloneTo(SR_UTILS_NS::SRClass& clone) const override;

        void SetStateMachine(AnimationStateMachine* pMachine);
        void Compile(CompileContext& context) override;
        bool SetSimpleClip(const SR_HTYPES_NS::SharedPtr<AnimationClip>& pClip);

        SR_NODISCARD bool IsStateActive(SR_UTILS_NS::StringAtom name) const override;
        SR_NODISCARD AnimationPose* Update(UpdateContext& context, const AnimationLink& from) override;

        SR_NODISCARD const SR_HTYPES_NS::SharedPtr<AnimationStateMachine>& GetMachine() const noexcept { return m_stateMachine; }

    protected:
        /// @property @notNull
        SR_HTYPES_NS::SharedPtr<AnimationStateMachine> m_stateMachine;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationGraphNodeExternalPose : public AnimationGraphNode {
        SR_CLASS()
        using Super = AnimationGraphNode;
    public:
        AnimationGraphNodeExternalPose();

        SR_NODISCARD AnimationPose* Update(UpdateContext& context, const AnimationLink& from) override;

    protected:
        /// @property
        SR_UTILS_NS::StringAtom m_externalId;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationGraphNodeLinearBlend : public AnimationGraphNode {
        SR_CLASS()
        using Super = AnimationGraphNode;
    public:
        AnimationGraphNodeLinearBlend();

        void Compile(CompileContext& context) override;

        SR_NODISCARD AnimationPose* Update(UpdateContext& context, const AnimationLink& from) override;

    private:
        std::array<AnimationPose*, 2> m_poses;

        /// @property
        float_t m_blendFactor = 0.5f;
        /// @property
        QuaternionBlendMode m_blendMode = QuaternionBlendMode::Nlerp;

    };
    /// ----------------------------------------------------------------------------------------------------------------
}

#endif //SR_ENGINE_ANIMATIONGRAPHNODE_H
