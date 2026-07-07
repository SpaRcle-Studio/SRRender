//
// Created by Monika on 14.01.2023.
//

#include <Graphics/GUI/NodeWidget.h>
#include <Graphics/GUI/NodeCreation.h>
#include <Graphics/GUI/Node.h>
#include <Graphics/GUI/Link.h>
#include <Graphics/Types/Texture.h>

#include <ImmediateGUI/GUI/ImmediateGUI.h>

#include <Utils/SRLM/LogicalMachine.h>
#include <Utils/SRLM/LogicalNodeManager.h>

#include <Codegen/NodeWidget.generated.hpp>

namespace SR_GRAPH_GUI_NS {
        void INodeWidgetDataContainer::RemoveLink(Link *pLink) {
        pLink->Broke(nullptr);
    }

    void INodeWidgetDataContainer::RemoveNode(Node* pNode) {
        auto&& pIt = m_nodes.find(pNode->GetId());
        if (pIt == m_nodes.end()) {
            SRHalt0();
            return;
        }

        m_nodes.erase(pIt);
        delete pNode;
    }

    Node& INodeWidgetDataContainer::AddNode(Node* pNode) {
        static Node def;

        if (!pNode) {
            SRHalt0();
            return def;
        }

        if (!CanAddNode(pNode)) {
            return def;
        }

        SRAssert(m_nodes.count(pNode->GetId()) == 0);
        m_nodes.insert(std::make_pair(pNode->GetId(), pNode));

        return *pNode;
    }

    Link& INodeWidgetDataContainer::AddLink(Link* pLink) {
        if (!pLink) {
            SRHalt0();
            static Link def;
            return def;
        }

        SRAssert(m_links.count(pLink->GetId()) == 0);
        m_links.insert(std::make_pair(pLink->GetId(), pLink));

        return *pLink;
    }

    void INodeWidgetDataContainer::ClearContainer() {
        for (auto&& [id, pLink] : m_links) {
            delete pLink;
        }

        for (auto&& [id, pNode] : m_nodes) {
            delete pNode;
        }

        m_nodes.clear();
        m_links.clear();
    }

    NodeWidget::NodeWidget(std::string name, SR_MATH_NS::IVector2 size)
        : Super(std::move(name), size)
    { }

    NodeWidget::~NodeWidget() {
        Clear();
    }

    void NodeWidget::Init() {
        Super::Init();
    }

    void NodeWidget::UpdateTouch() {

    }

    void NodeWidget::Clear() {
        ClearContainer();
    }

    void NodeWidget::Draw() {

    }

    void NodeWidget::OnClose() {
        Clear();

        Super::OnClose();
    }

    void NodeWidget::DrawTopPanel() {
        SR_GRAPH_GUI_NS::Immediate::PushStyleVar(SR_GRAPH_GUI_NS::Immediate::StyleVar::ChildRounding, 0);
        SR_GRAPH_GUI_NS::Immediate::PushStyleVar(SR_GRAPH_GUI_NS::Immediate::StyleVar::WindowRounding, 0);
        SR_GRAPH_GUI_NS::Immediate::PushStyleVar(SR_GRAPH_GUI_NS::Immediate::StyleVar::FrameRounding, 0);

        if (SR_GRAPH_GUI_NS::Immediate::Button("Open")) {
            TopPanelOpen();
        }

        SR_GRAPH_GUI_NS::Immediate::SameLine();

        if (SR_GRAPH_GUI_NS::Immediate::Button("Save")) {
            TopPanelSave();
        }

        SR_GRAPH_GUI_NS::Immediate::SameLine();

        if (SR_GRAPH_GUI_NS::Immediate::Button("Save at")) {
            TopPanelSaveAt();
        }

        SR_GRAPH_GUI_NS::Immediate::SameLine();

        SR_GRAPH_GUI_NS::Immediate::Text(" | ");

        SR_GRAPH_GUI_NS::Immediate::SameLine();

        if (SR_GRAPH_GUI_NS::Immediate::Button("Zoom")) {
            Zoom();
        }

        SR_GRAPH_GUI_NS::Immediate::SameLine();

        SR_GRAPH_GUI_NS::Immediate::Text(" | ");

        SR_GRAPH_GUI_NS::Immediate::SameLine();

        if (SR_GRAPH_GUI_NS::Immediate::Button("Execute")) {
            Execute();
        }

        SR_GRAPH_GUI_NS::Immediate::SameLine();

        SR_GRAPH_GUI_NS::Immediate::Text(" | ");

        SR_GRAPH_GUI_NS::Immediate::SameLine();

        if (SR_GRAPH_GUI_NS::Immediate::Button("Close")) {
            TopPanelClose();
        }

        SR_GRAPH_GUI_NS::Immediate::SameLine();

        SR_GRAPH_GUI_NS::Immediate::Text(" | %s", m_currentFile.empty() ? " [Non saved]" : m_currentFile.CStr());

        SR_GRAPH_GUI_NS::Immediate::SameLine();
        SR_GRAPH_GUI_NS::Immediate::PopStyleVar(3);
    }

    void NodeWidget::DrawPopupMenu() {

    }

    void NodeWidget::TopPanelSaveAt() {

    }

    void NodeWidget::TopPanelOpen() {

    }

    void NodeWidget::TopPanelSave() {

    }

    void NodeWidget::Execute() {

    }

    void NodeWidget::TopPanelClose() {
        Close();
    }

    void NodeWidget::DrawLeftPanel() {

    }

    void NodeWidget::DrawNodeEditor() {

    }
}
