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
        explicit AnimationGraphNode(uint16_t input, uint16_t output);
        ~AnimationGraphNode() override;

    public:
        SR_NODISCARD static AnimationGraphNode* Load(const SR_XML_NS::Node& nodeXml);

        SR_NODISCARD uint32_t GetInputCount() const noexcept { return static_cast<uint32_t>(m_inputPins.size()); }
        SR_NODISCARD uint32_t GetOutputCount() const noexcept { return static_cast<uint32_t>(m_outputPins.size()); }
        SR_NODISCARD virtual AnimationGraphNodeType GetType() const noexcept = 0;
        virtual SR_NODISCARD AnimationPose* Update(UpdateContext& context, const AnimationLink& from) = 0;
        virtual void Compile(CompileContext& context) { }

        void SetGraph(AnimationGraph* pGraph) { m_graph = pGraph; }

        SR_NODISCARD uint64_t GetIndex() const;

        void ConnectTo(AnimationGraphNode* pNode, uint16_t fromPinIndex, uint16_t toPinIndex);

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
        explicit AnimationGraphNodeFinal()
            : Super(1, 0)
        { }

    public:
        SR_NODISCARD AnimationPose* Update(UpdateContext& context, const AnimationLink& from) override;

        SR_NODISCARD AnimationGraphNodeType GetType() const noexcept override { return AnimationGraphNodeType::Final; }

    };

    /// ----------------------------------------------------------------------------------------------------------------

    class AnimationGraphNodeStateMachine : public AnimationGraphNode {
        using Super = AnimationGraphNode;
    public:
        AnimationGraphNodeStateMachine()
            :  Super(0, 1)
        { }

        ~AnimationGraphNodeStateMachine() override;

    public:
        SR_NODISCARD static AnimationGraphNodeStateMachine* Load(const SR_XML_NS::Node& nodeXml);

        void SetStateMachine(AnimationStateMachine* pMachine);

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
