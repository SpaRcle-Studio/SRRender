//
// Created by Monika on 14.01.2023.
//

#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Types/Texture.h>
#include <Graphics/GUI/Pin.h>
#include <Graphics/GUI/Node.h>
#include <Graphics/GUI/NodeBuilder.h>
#include <Graphics/GUI/ImNodeEditorUtils.h>
#include <Graphics/GUI/ImmediateGUI.h>

namespace SR_GRAPH_GUI_NS {
    NodeBuilder::NodeBuilder(SR_GTYPES_NS::Texture *pTexture)
        : m_texture(pTexture)
    {
        if (m_texture) {
            m_texture->AddUsePoint();
        }
    }

    NodeBuilder::~NodeBuilder() {
        if (m_texture) {
            m_texture->RemoveUsePoint();
        }
    }

    void NodeBuilder::Begin(Node* pNode) {
    #ifdef SR_USE_IMGUI_NODE_EDITOR
        if (!pNode) {
            return;
        }

        m_hasHeader = false;
        m_headerMin = m_headerMax = SR_MATH_NS::FVector2();

        SR_GRAPH_GUI_NS::Immediate::PushNodeEditorStyleVar(
            SR_GRAPH_GUI_NS::Immediate::NodeEditorStyleVar::NodePadding,
            SR_MATH_NS::FVector4(8, 8, 8, 8)
        );

        SR_GRAPH_GUI_NS::Immediate::BeginNode(pNode->GetId());

        SR_GRAPH_GUI_NS::Immediate::PushID(reinterpret_cast<const void*>(pNode->GetId()));
        m_currentNodeId = pNode->GetId();
        m_currentNode = pNode;

        SetStage(Stage::Begin);
    #endif
    }

    //inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
    //    return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
    //}

    //inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
    //    return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
    //}

    void NodeBuilder::End() {
    #ifdef SR_USE_IMGUI_NODE_EDITOR
        SetStage(Stage::End);

        SR_GRAPH_GUI_NS::Immediate::EndNode();

        // Рисуем цветной заголовок на фоне ноды
        if (SR_GRAPH_GUI_NS::Immediate::IsItemVisible()) {
            auto&& drawList = SR_GRAPH_GUI_NS::Immediate::GetNodeBackgroundDrawList(m_currentNodeId);
            if (drawList) {
                // Получаем alpha из текущего стиля
                auto&& alpha = static_cast<int>(255); // TODO: получить из ImGui::GetStyle().Alpha
                const auto halfBorderWidth = 0.75f; // TODO: получить из ed::GetStyle().NodeBorderWidth * 0.5f

                if ((m_headerMax.x > m_headerMin.x) && (m_headerMax.y > m_headerMin.y)) {
                    // Рисуем цветной заголовок
                    auto&& headerColorU32 = SR_COL32(
                        static_cast<int>(m_headerColor.r * 255),
                        static_cast<int>(m_headerColor.g * 255),
                        static_cast<int>(m_headerColor.b * 255),
                        alpha
                    );

                    // Конвертируем FVector2 в ImVec2
                    SR_MATH_NS::FVector2 headerMin(m_headerMin.x, m_headerMin.y);
                    SR_MATH_NS::FVector2 headerMax(m_headerMax.x, m_headerMax.y);

                    // Рисуем прямоугольник заголовка с закругленными верхними углами
                    SR_GRAPH_GUI_NS::Immediate::DrawListAddRectFilled(drawList,
                        headerMin - SR_MATH_NS::FVector2(8 - halfBorderWidth, 4 - halfBorderWidth),
                        headerMax + SR_MATH_NS::FVector2(8 - halfBorderWidth, 0), headerColorU32, 8.0f, static_cast<Immediate::DrawFlags>(1 | 2)); // Закругленные верхние углы

                    // Разделитель между заголовком и содержимым
                    auto&& headerSeparatorMin = SR_MATH_NS::FVector2(headerMin.x, headerMax.y);
                    auto&& headerSeparatorMax = SR_MATH_NS::FVector2(headerMax.x, headerMin.y);
                    if ((headerSeparatorMax.x > headerSeparatorMin.x) && (headerSeparatorMax.y > headerSeparatorMin.y)) {
                        SR_GRAPH_GUI_NS::Immediate::DrawListAddLine(drawList,
                            headerSeparatorMin + SR_MATH_NS::FVector2(-(8 - halfBorderWidth), -0.5f),
                            headerSeparatorMax + SR_MATH_NS::FVector2((8 - halfBorderWidth), -0.5f),
                            SR_COL32(255, 255, 255, 96 * alpha / (3 * 255)), 1.0f);
                    }
                }
            }
        }

        m_currentNodeId = 0;
        m_currentNode = nullptr;

        SR_GRAPH_GUI_NS::Immediate::PopID();

        SR_GRAPH_GUI_NS::Immediate::PopNodeEditorStyleVar(1);

        SetStage(Stage::Invalid);
    #endif
    }

    void NodeBuilder::Header(const SR_MATH_NS::FColor& color) {
        m_headerColor = color;
        SetStage(Stage::Header);
    }

    void NodeBuilder::EndHeader() {
        SetStage(Stage::Content);
    }

    void NodeBuilder::Input(Pin* pPin) {
    #ifdef SR_USE_IMGUI_NODE_EDITOR
        if (!pPin) {
            return;
        }

        if (m_currentStage == Stage::Begin)
            SetStage(Stage::Content);

        const auto applyPadding = (m_currentStage == Stage::Input);

        SetStage(Stage::Input);

        if (applyPadding)
            SR_GRAPH_GUI_NS::Immediate::Spring(0);

        // Начинаем пин
        SR_GRAPH_GUI_NS::Immediate::BeginPin(pPin->GetId(), true);
        SR_GRAPH_GUI_NS::Immediate::PinPivotAlignment(SR_MATH_NS::FVector2(0, 0.5f));
        SR_GRAPH_GUI_NS::Immediate::PinPivotSize(SR_MATH_NS::FVector2(0, 0));
        m_currentPin = pPin;

        SR_GRAPH_GUI_NS::Immediate::BeginHorizontal(reinterpret_cast<const void*>(pPin->GetId()));
    #endif
    }

    void NodeBuilder::EndInput() {
    #ifdef SR_USE_IMGUI_NODE_EDITOR
        SR_GRAPH_GUI_NS::Immediate::EndHorizontal();

        SR_GRAPH_GUI_NS::Immediate::EndPin();
        m_currentPin = nullptr;
    #endif
    }

    void NodeBuilder::Middle() {
        if (m_currentStage == Stage::Begin)
            SetStage(Stage::Content);

        SetStage(Stage::Middle);
    }

    void NodeBuilder::Output(Pin* pPin) {
    #ifdef SR_USE_IMGUI_NODE_EDITOR
        if (!pPin) {
            return;
        }

        if (m_currentStage == Stage::Begin)
            SetStage(Stage::Content);

        const auto applyPadding = (m_currentStage == Stage::Output);

        SetStage(Stage::Output);

        if (applyPadding)
            SR_GRAPH_GUI_NS::Immediate::Spring(0);

        // Начинаем пин
        SR_GRAPH_GUI_NS::Immediate::BeginPin(pPin->GetId(), false);
        SR_GRAPH_GUI_NS::Immediate::PinPivotAlignment(SR_MATH_NS::FVector2(1.0f, 0.5f));
        SR_GRAPH_GUI_NS::Immediate::PinPivotSize(SR_MATH_NS::FVector2(0, 0));
        m_currentPin = pPin;

        SR_GRAPH_GUI_NS::Immediate::BeginHorizontal(reinterpret_cast<const void*>(pPin->GetId()));
    #endif
    }

    void NodeBuilder::EndOutput() {
    #ifdef SR_USE_IMGUI_NODE_EDITOR
        SR_GRAPH_GUI_NS::Immediate::EndHorizontal();

        SR_GRAPH_GUI_NS::Immediate::EndPin();
        m_currentPin = nullptr;
    #endif
    }

    bool NodeBuilder::SetStage(NodeBuilder::Stage stage) {
        if (stage == m_currentStage)
            return false;

        auto oldStage = m_currentStage;
        m_currentStage = stage;

        // Обработка завершения предыдущего этапа
        switch (oldStage) {
            case Stage::Begin:
                break;

            case Stage::Header:
                SR_GRAPH_GUI_NS::Immediate::EndHorizontal();
                m_headerMin = SR_GRAPH_GUI_NS::Immediate::GetItemRectMin();
                m_headerMax = SR_GRAPH_GUI_NS::Immediate::GetItemRectMax();
                // Отступ между заголовком и содержимым
                SR_GRAPH_GUI_NS::Immediate::Spring(0, SR_GRAPH_GUI_NS::Immediate::GetFrameHeightWithSpacing() * 2.0f);
                break;

            case Stage::Content:
                break;

            case Stage::Input:
                SR_GRAPH_GUI_NS::Immediate::PopNodeEditorStyleVar(2);
                SR_GRAPH_GUI_NS::Immediate::Spring(1, 0);
                SR_GRAPH_GUI_NS::Immediate::EndVertical();
                break;

            case Stage::Middle:
                SR_GRAPH_GUI_NS::Immediate::EndVertical();
                break;

            case Stage::Output:
                SR_GRAPH_GUI_NS::Immediate::PopNodeEditorStyleVar(2);
                SR_GRAPH_GUI_NS::Immediate::Spring(1, 0);
                SR_GRAPH_GUI_NS::Immediate::EndVertical();
                break;

            case Stage::End:
                break;

            case Stage::Invalid:
                break;
        }

        // Обработка начала нового этапа
        switch (stage) {
            case Stage::Begin:
                SR_GRAPH_GUI_NS::Immediate::BeginVertical("node");
                break;

            case Stage::Header:
                m_hasHeader = true;
                SR_GRAPH_GUI_NS::Immediate::BeginHorizontal("header");
                break;

            case Stage::Content:
                if (oldStage == Stage::Begin)
                    SR_GRAPH_GUI_NS::Immediate::Spring(0);
                SR_GRAPH_GUI_NS::Immediate::BeginHorizontal("content");
                SR_GRAPH_GUI_NS::Immediate::Spring(0, 0);
                break;

            case Stage::Input:
                SR_GRAPH_GUI_NS::Immediate::BeginVertical("inputs", SR_MATH_NS::FVector2(0, 0), 0.0f);
                SR_GRAPH_GUI_NS::Immediate::PushNodeEditorStyleVar(
                    SR_GRAPH_GUI_NS::Immediate::NodeEditorStyleVar::SourceDirection,
                    SR_MATH_NS::FVector2(0, 0.5f)
                );
                SR_GRAPH_GUI_NS::Immediate::PushNodeEditorStyleVar(
                    SR_GRAPH_GUI_NS::Immediate::NodeEditorStyleVar::TargetDirection,
                    SR_MATH_NS::FVector2(0, 0)
                );
                if (!m_hasHeader)
                    SR_GRAPH_GUI_NS::Immediate::Spring(1, 0);
                break;

            case Stage::Middle:
                SR_GRAPH_GUI_NS::Immediate::Spring(1);
                SR_GRAPH_GUI_NS::Immediate::BeginVertical("middle", SR_MATH_NS::FVector2(0, 0), 1.0f);
                break;

            case Stage::Output:
                if (oldStage == Stage::Middle || oldStage == Stage::Input)
                    SR_GRAPH_GUI_NS::Immediate::Spring(1);
                else
                    SR_GRAPH_GUI_NS::Immediate::Spring(1, 0);
                SR_GRAPH_GUI_NS::Immediate::BeginVertical("outputs", SR_MATH_NS::FVector2(0, 0), 1.0f);
                SR_GRAPH_GUI_NS::Immediate::PushNodeEditorStyleVar(
                    SR_GRAPH_GUI_NS::Immediate::NodeEditorStyleVar::SourceDirection,
                    SR_MATH_NS::FVector2(1.0f, 0.5f)
                );
                SR_GRAPH_GUI_NS::Immediate::PushNodeEditorStyleVar(
                    SR_GRAPH_GUI_NS::Immediate::NodeEditorStyleVar::TargetDirection,
                    SR_MATH_NS::FVector2(0, 0)
                );
                if (!m_hasHeader)
                    SR_GRAPH_GUI_NS::Immediate::Spring(1, 0);
                break;

            case Stage::End:
                if (oldStage == Stage::Input)
                    SR_GRAPH_GUI_NS::Immediate::Spring(1, 0);
                if (oldStage != Stage::Begin)
                    SR_GRAPH_GUI_NS::Immediate::EndHorizontal();
                m_contentMin = SR_GRAPH_GUI_NS::Immediate::GetItemRectMin();
                m_contentMax = SR_GRAPH_GUI_NS::Immediate::GetItemRectMax();
                SR_GRAPH_GUI_NS::Immediate::EndVertical();
                m_nodeMin = SR_GRAPH_GUI_NS::Immediate::GetItemRectMin();
                m_nodeMax = SR_GRAPH_GUI_NS::Immediate::GetItemRectMax();
                break;

            case Stage::Invalid:
                break;
        }

        return true;
    }
}

