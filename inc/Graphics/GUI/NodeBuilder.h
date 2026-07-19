//
// Created by Monika on 14.01.2023.
//

#ifndef SR_ENGINE_NODEBUILDER_H
#define SR_ENGINE_NODEBUILDER_H

#include <Graphics/stdInclude.h>

#include <Utils/Common/NonCopyable.h>
#include <Utils/Math/Vector4.h>

namespace SR_GTYPES_NS {
    class Texture;
}

namespace SR_GRAPH_NS::GUI {
    class Node;
    class Pin;
    class Link;

    class NodeBuilder : public SR_UTILS_NS::NonCopyable {
        enum class Stage : uint8_t {
            Invalid,
            Begin,
            Header,
            Content,
            Input,
            Output,
            Middle,
            End
        };
    public:
        explicit NodeBuilder(SR_GTYPES_NS::Texture* pTexture);
        ~NodeBuilder() override;

        void Begin(Node* pNode);
        void End();

        void Header(const SR_MATH_NS::FColor& color = SR_MATH_NS::FColor(1, 1, 1, 1));
        void EndHeader();

        void Input(Pin* pPin);
        void EndInput();

        void Middle();

        void Output(Pin* pPin);
        void EndOutput();

    private:
        bool SetStage(Stage stage);

    private:
        SR_GTYPES_NS::Texture* m_texture = nullptr;

        uintptr_t m_currentNodeId = 0;

        Node* m_currentNode = nullptr;
        Pin* m_currentPin = nullptr;

        Stage m_currentStage = Stage::Invalid;
        SR_MATH_NS::FColor m_headerColor = SR_MATH_NS::FColor(1, 1, 1, 1);
        SR_MATH_NS::FVector2 m_headerMin;
        SR_MATH_NS::FVector2 m_headerMax;
        SR_MATH_NS::FVector2 m_contentMin;
        SR_MATH_NS::FVector2 m_contentMax;
        SR_MATH_NS::FVector2 m_nodeMin;
        SR_MATH_NS::FVector2 m_nodeMax;

        bool m_hasHeader = false;

    };

}

#endif //SR_ENGINE_NODEBUILDER_H
