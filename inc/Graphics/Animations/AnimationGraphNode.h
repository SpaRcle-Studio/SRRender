//
// Created by Monika on 06.05.2023.
//

#ifndef SR_ENGINE_ANIMATIONGRAPHNODE_H
#define SR_ENGINE_ANIMATIONGRAPHNODE_H

#include <Graphics/Animations/AnimationCommon.h>
#include <Graphics/Animations/AnimationContext.h>

namespace SR_ANIMATIONS_NS {
    class AnimationStateMachine;
    class AnimationGraph;
    class AnimationPose;

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationGraphNode : public SR_UTILS_NS::NonCopyable {
    public:
        /**
         * @param input - сколько данная нода имеет входных пинов
         * @param output - сколько данная нода имеет выходных пинов
         */
        explicit AnimationGraphNode(AnimationGraph* pGraph, uint16_t input, uint16_t output);
        ~AnimationGraphNode() override;

    public:
        SR_NODISCARD virtual AnimationGraphNodeType GetType() const noexcept = 0;
        virtual SR_NODISCARD AnimationPose* Update(UpdateContext& context, const AnimationLink& from) = 0;
        virtual void Compile(CompileContext& context) { }

        SR_NODISCARD uint64_t GetIndex() const;

        /**
          * Добавляет в inputPins ноду pNode по индексу sourcePinIndex.
          * В pNode добавляет в outputPins ноду this по индексу targetPinIndex.
        */
        void AddInput(AnimationGraphNode* pNode, uint16_t sourcePinIndex, uint16_t targetPinIndex);

    protected:
        AnimationGraph* m_graph = nullptr;
        AnimationPose* m_pose = nullptr;

        std::vector<std::optional<AnimationLink>> m_inputPins;
        std::vector<std::optional<AnimationLink>> m_outputPins;

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationGraphNodeFinal : public AnimationGraphNode {
        using Super = AnimationGraphNode;
    public:
        explicit AnimationGraphNodeFinal(AnimationGraph* pGraph)
            : Super(pGraph, 1, 0)
        { }

    public:
        SR_NODISCARD AnimationPose* Update(UpdateContext& context, const AnimationLink& from) override;

        SR_NODISCARD AnimationGraphNodeType GetType() const noexcept override { return AnimationGraphNodeType::Final; }

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationGraphNodeStateMachine : public AnimationGraphNode {
        using Super = AnimationGraphNode;
    public:
        explicit AnimationGraphNodeStateMachine(AnimationGraph* pGraph);
        ~AnimationGraphNodeStateMachine() override;

    public:
        SR_NODISCARD AnimationPose* Update(UpdateContext& context, const AnimationLink& from) override;
        void Compile(CompileContext& context) override;

        SR_NODISCARD AnimationStateMachine* GetMachine() const noexcept { return m_stateMachine; }
        SR_NODISCARD AnimationGraphNodeType GetType() const noexcept override { return AnimationGraphNodeType::StateMachine; }

    protected:
        AnimationStateMachine* m_stateMachine = nullptr;

    };

    /// ----------------------------------------------------------------------------------------------------------------
}

#endif //SR_ENGINE_ANIMATIONGRAPHNODE_H
