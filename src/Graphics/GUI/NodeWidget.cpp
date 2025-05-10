//
// Created by Monika on 14.01.2023.
//

#include <Graphics/GUI/NodeWidget.h>
#include <Graphics/GUI/NodeCreation.h>
#include <Graphics/Types/Texture.h>

#include <Utils/SRLM/LogicalMachine.h>
#include <Utils/SRLM/LogicalNodes.h>
#include <Utils/SRLM/DataTypeManager.h>
#include <Utils/SRLM/LogicalNodeManager.h>
#include <Utils/Common/HashManager.h>
#include <Utils/FileSystem/FileDialog.h>

namespace SR_GRAPH_GUI_NS {
    NodeWidget::NodeWidget(std::string name, SR_MATH_NS::IVector2 size)
        : Super(std::move(name), size)
    {
        m_nodeBuilder = new NodeBuilder(SR_GTYPES_NS::Texture::Load("Editor/Textures/BlueprintBackground.png"));

        m_creationPopup = new PopupItemSubWidget(GetName() + "-Popup");
    }

    NodeWidget::~NodeWidget() {
        Clear();
        delete m_nodeBuilder;
    }

    void NodeWidget::UpdateTouch() {

    }

    void NodeWidget::Clear() {
        for (auto&& [id, pLink] : m_links) {
            delete pLink;
        }

        for (auto&& [id, pNode] : m_nodes) {
            delete pNode;
        }

        m_nodes.clear();
        m_links.clear();

        m_currentFile = SR_UTILS_NS::Path();
    }

    void NodeWidget::Draw() {

    }

    void NodeWidget::RemoveLink(Link *pLink) {
        pLink->Broke(nullptr);
    }

    void NodeWidget::RemoveNode(Node* pNode) {
        auto&& pIt = m_nodes.find(pNode->GetId());
        if (pIt == m_nodes.end()) {
            SRHalt0();
            return;
        }

        m_nodes.erase(pIt);
        delete pNode;
    }

    Node& NodeWidget::AddNode(Node* pNode) {
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

    Link& NodeWidget::AddLink(Link* pLink) {
        if (!pLink) {
            SRHalt0();
            static Link def;
            return def;
        }

        SRAssert(m_links.count(pLink->GetId()) == 0);
        m_links.insert(std::make_pair(pLink->GetId(), pLink));

        return *pLink;
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

    void NodeWidget::Init() {
        InitStructsCreationPopup();
        InitCreationPopup();
        Super::Init();
    }

    void NodeWidget::InitStructsCreationPopup() {
        auto&& menu = m_creationPopup->AddMenu("Structs");

        /*for (auto&& [hashName, pStruct] : SR_SRLM_NS::DataTypeManager::Instance().GetStructs()) {
            auto&& structMenu = menu.AddMenu(std::string(SR_HASH_TO_STR(hashName)));

            structMenu.AddMenu("Create").SetAction([typeHashName = hashName](const SR_GRAPH_GUI_NS::DrawPopupContext& context) {
                auto&& pNode = new SR_SRLM_NS::CreateStructNode();
                pNode->SetStructHashName(typeHashName);
                pNode->InitNode();
                context.pWidget->AddNode(new Node(pNode)).SetPosition(context.popupPos);
            });

            structMenu.AddMenu("Break").SetAction([typeHashName = hashName](const SR_GRAPH_GUI_NS::DrawPopupContext& context) {
                auto&& pNode = new SR_SRLM_NS::BreakStructNode();
                pNode->SetStructHashName(typeHashName);
                pNode->InitNode();
                context.pWidget->AddNode(new Node(pNode)).SetPosition(context.popupPos);
            });
        }*/
    }

    void NodeWidget::InitCreationPopup() {
        /*for (auto&& [hashName, nodeInfo] : SR_SRLM_NS::LogicalNodeManager::Instance().GetNodeConstructors()) {
            if (nodeInfo.category.empty()) {
                continue;
            }

            auto&& menu = m_creationPopup->AddMenu(nodeInfo.category);
            menu.AddMenu(std::string(SR_HASH_TO_STR(hashName))).SetAction([constructor = nodeInfo.constructor](const SR_GRAPH_GUI_NS::DrawPopupContext& context) {
                SR_SRLM_NS::LogicalNode* pLogicalNode = constructor();
                pLogicalNode->InitNode();
                pLogicalNode->InitValues();
                context.pWidget->AddNode(new Node(pLogicalNode)).SetPosition(context.popupPos);
            });
        }*/
    }

    void NodeWidget::TopPanelSaveAt() {
        /*auto&& path = SR_UTILS_NS::FileDialog::Instance().SaveDialog(SR_UTILS_NS::ResourceManager::Instance().GetResPath(), { { "SRLM", "srlm" } });
        if (path.empty()) {
            return;
        }

        m_currentFile = path;

        TopPanelSave();*/
    }

    void NodeWidget::TopPanelOpen() {
        /*auto&& path = SR_UTILS_NS::FileDialog::Instance().OpenDialog(SR_UTILS_NS::ResourceManager::Instance().GetResPath(), { { "SRLM", "srlm" } });
        if (path.empty()) {
            return;
        }

        Clear();

        m_currentFile = path;

        auto&& xmlDocument = SR_XML_NS::Document::Load(path);
        if (!xmlDocument) {
            return;
        }

        auto&& xmlLogicalMachine = xmlDocument.Root().GetNode("LogicalMachine");

        std::map<uint64_t, Node*> nodes;

        auto&& xmlNodes = xmlLogicalMachine.GetNode("Nodes");
        for (auto&& xmlNode : xmlNodes.GetNodes()) {
            auto&& uid = xmlNode.GetAttribute("UID").ToUInt64();
            if (auto&& pLogicalNode = SR_SRLM_NS::LogicalNode::LoadXml(xmlNode)) {
                auto&& pNode = new Node(pLogicalNode);
                pNode->SetPosition(xmlNode.GetAttribute<SR_MATH_NS::FVector2>());
                nodes[uid] = pNode;
            }
        }

        auto&& xmlLinks = xmlLogicalMachine.GetNode("Links");
        for (auto&& xmlLink : xmlLinks.GetNodes()) {
            auto&& startNodeId = xmlLink.GetAttribute("SN").ToUInt64();
            auto&& endNodeId = xmlLink.GetAttribute("EN").ToUInt64();

            auto&& startPinIndex = xmlLink.GetAttribute("SP").ToUInt64();
            auto&& endPinIndex = xmlLink.GetAttribute("EP").ToUInt64();

            if (nodes.count(startNodeId) == 0) {
                SR_ERROR("NodeWidget::TopPanelOpen() : start node not exists! Id: " + SR_UTILS_NS::ToString(startNodeId));
                continue;
            }

            if (nodes.count(endNodeId) == 0) {
                SR_ERROR("NodeWidget::TopPanelOpen() : end node not exists! Id: " + SR_UTILS_NS::ToString(endNodeId));
                continue;
            }

            AddLink(new Link(
                nodes[startNodeId]->GetOutputPin(startPinIndex),
                nodes[endNodeId]->GetInputPin(endPinIndex)
            ));
        }

        for (auto&& [uid, pNode] : nodes) {
            AddNode(pNode);
        }*/
    }

    void NodeWidget::TopPanelSave() {
        /*if (m_currentFile.empty()) {
            TopPanelSaveAt();
            return;
        }

        auto&& xmlDocument = SR_XML_NS::Document::New();

        auto&& xmlLogicalMachine = xmlDocument.Root().AppendNode("LogicalMachine");
        auto&& xmlNodes = xmlLogicalMachine.AppendNode("Nodes");

        for (auto&& [uid, pNode] : m_nodes) {
            auto&& xmlNode = xmlNodes.AppendNode("Node");
            xmlNode.AppendAttribute("UID", uid);
            xmlNode.AppendAttribute(pNode->GetPosition());
            pNode->GetLogicalNode()->SaveXml(xmlNode);
        }

        auto&& xmlLinks = xmlLogicalMachine.AppendNode("Links");

        for (auto&& [uid, pLink] : m_links) {
            if (!pLink->IsLinked()) {
                continue;
            }

            auto&& xmlLink = xmlLinks.AppendChild("Link");

            xmlLink.AppendAttribute("SN", pLink->GetStart()->GetNode()->GetId());
            xmlLink.AppendAttribute("EN", pLink->GetEnd()->GetNode()->GetId());

            xmlLink.AppendAttribute("SP", pLink->GetStart()->GetIndex());
            xmlLink.AppendAttribute("EP", pLink->GetEnd()->GetIndex());
        }

        xmlDocument.Save(m_currentFile);*/
    }

    void NodeWidget::Execute() {
        /*if (m_nodes.empty() && m_properties.empty()) {
            TopPanelOpen();
        }
        else {
            TopPanelSave();
        }

        if (auto&& pLogicalMachine = SR_SRLM_NS::LogicalMachine::Load(m_currentFile)) {
            pLogicalMachine->AddUsePoint();
            pLogicalMachine->Init();
            pLogicalMachine->UpdateMachine(0.f);
            pLogicalMachine->RemoveUsePoint();
        }*/
    }

    void NodeWidget::TopPanelClose() {
        Close();
    }

    void NodeWidget::DrawLeftPanel() {

    }

    void NodeWidget::DrawNodeEditor() {

    }

    NodeWidgetProperty* NodeWidget::FindProperty(const std::string& name) {
        for (auto&& property : m_properties) {
            if (property.name == name) {
                return &property;
            }
        }
        return nullptr;
    }
}
