//
// Created by Monika on 14.01.2023.
//

#include <Graphics/GUI/NodeWidget.h>
#include <Graphics/GUI/NodeCreation.h>
#include <Graphics/GUI/Node.h>
#include <Graphics/GUI/Link.h>
#include <Graphics/GUI/ImGUI.h>
#include <Graphics/Types/Texture.h>

#include <Utils/SRLM/LogicalMachine.h>
#include <Utils/SRLM/LogicalNodes.h>
#include <Utils/SRLM/DataTypeManager.h>
#include <Utils/SRLM/LogicalNodeManager.h>
#include <Utils/Common/HashManager.h>
#include <Utils/FileSystem/FileDialog.h>

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
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

        if (ImGui::Button("Open")) {
            TopPanelOpen();
        }

        ImGui::SameLine();

        if (ImGui::Button("Save")) {
            TopPanelSave();
        }

        ImGui::SameLine();

        if (ImGui::Button("Save at")) {
            TopPanelSaveAt();
        }

        ImGui::SameLine();

        ImGui::Text(" | ");

        ImGui::SameLine();

        if (ImGui::Button("Zoom")) {
            Zoom();
        }

        ImGui::SameLine();

        ImGui::Text(" | ");

        ImGui::SameLine();

        if (ImGui::Button("Execute")) {
            Execute();
        }

        ImGui::SameLine();

        ImGui::Text(" | ");

        ImGui::SameLine();

        if (ImGui::Button("Close")) {
            TopPanelClose();
        }

        ImGui::SameLine();

        ImGui::Text(" | %s", m_currentFile.empty() ? " [Non saved]" : m_currentFile.CStr());

        ImGui::SameLine();
        ImGui::PopStyleVar(3);
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
