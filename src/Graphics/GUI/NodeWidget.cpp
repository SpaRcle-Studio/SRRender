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
#include <Utils/Common/StringUtils.h>

#include <Codegen/NodeWidget.generated.hpp>

namespace SR_GRAPH_GUI_NS {
    NodeWidget::NodeWidget()
        : Super()
        , m_nodeGraphEditor(SR_IMMEDIATE_GUI_NS::NodeEditorInstance::Create())
    { }

    NodeWidget::NodeWidget(std::string name, SR_MATH_NS::IVector2 size)
        : Super(std::move(name), size)
        , m_nodeGraphEditor(SR_IMMEDIATE_GUI_NS::NodeEditorInstance::Create())
    { }

    NodeWidget::~NodeWidget() {
        Clear();
    }

    void NodeWidget::Init() {
        Super::Init();
    }

    void NodeWidget::UpdateTouch() {

    }

    void NodeWidget::Zoom() {
        if (m_nodeGraphEditor) {
            m_nodeGraphEditor->Zoom();
        }
    }

    void NodeWidget::Clear() {

    }

    void NodeWidget::Draw() {
        DrawTopPanel();

        SR_GRAPH_GUI_NS::Immediate::Separator();

        auto&& availableSize = SR_GRAPH_GUI_NS::Immediate::GetContentRegionAvail();

        SR_GRAPH_GUI_NS::Immediate::BeginChild("InspectPanel", SR_MATH_NS::FVector2(m_leftPaneWidth, availableSize.y));
        DrawInspectPanel();
        SR_GRAPH_GUI_NS::Immediate::EndChild();

        SR_GRAPH_GUI_NS::Immediate::SameLine();

        SR_GRAPH_GUI_NS::Immediate::BeginChild("NodeEditor", SR_MATH_NS::FVector2(availableSize.x - m_leftPaneWidth - 10, availableSize.y));
        DrawNodeEditor();
        SR_GRAPH_GUI_NS::Immediate::EndChild();
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

    void NodeWidget::DrawInspectPanel() {

    }

    void NodeWidget::DrawNodeEditor() {

    }

    void NodeWidget::BuildNodeMenu(std::map<std::string, std::vector<SR_UTILS_NS::StringAtom>>& categories, SR_UTILS_NS::StringAtom baseClass) {
        SR_TRACY_ZONE;
        categories.clear();

        if (m_availableNodeTypes.empty()) {
            m_availableNodeTypes = SR_UTILS_NS::Factory::Instance().GetInheritances(baseClass);

            /// Фильтруем абстрактные и скрытые классы
            std::erase_if(m_availableNodeTypes, [](auto&& name) {
                auto&& pMeta = SR_UTILS_NS::Factory::Instance().GetType(name);
                if (!pMeta) {
                    return true;
                }
                return pMeta->IsAbstract() || pMeta->IsHidden();
            });
        }

        for (auto&& nodeTypeName : m_availableNodeTypes) {
            auto&& pMeta = SR_UTILS_NS::Factory::Instance().GetType(nodeTypeName);
            if (!pMeta) {
                continue;
            }

            auto&& category = pMeta->GetCategory();
            std::string categoryPath = "Nodes";
            if (!category.empty()) {
                categoryPath.clear();
                for (size_t i = 0; i < category.size(); ++i) {
                    if (i > 0) {
                        categoryPath += "/";
                    }
                    categoryPath += category[i].ToStringRef();
                }
            }

            categories[categoryPath].emplace_back(nodeTypeName);
        }
    }

    void NodeWidget::DrawNodeMenuRecursive(const std::map<std::string, std::vector<SR_UTILS_NS::StringAtom>>& categories, const std::string& prefix, SR_MATH_NS::FVector2 popupPos) {
        /// Группируем ноды по следующему уровню категорий
        std::map<std::string, std::vector<SR_UTILS_NS::StringAtom>> subCategories;
        std::vector<SR_UTILS_NS::StringAtom> directNodes;

        for (auto&& [categoryPath, nodeTypes] : categories) {
            if (categoryPath == prefix) {
                /// Ноды напрямую в этой категории
                directNodes.insert(directNodes.end(), nodeTypes.begin(), nodeTypes.end());
            }
            else if (prefix.empty() || (categoryPath.size() >= prefix.size() + 1 && categoryPath.substr(0, prefix.size() + 1) == prefix + "/")) {
                /// Определяем следующий уровень
                std::string remaining = prefix.empty() ? categoryPath : categoryPath.substr(prefix.size() + 1);
                auto&& nextSlash = remaining.find('/');

                if (nextSlash == std::string::npos) {
                    /// Это конечный уровень для этой категории
                    directNodes.insert(directNodes.end(), nodeTypes.begin(), nodeTypes.end());
                }
                else {
                    /// Есть подкатегория
                    std::string nextLevel = prefix.empty() ? remaining.substr(0, nextSlash) : prefix + "/" + remaining.substr(0, nextSlash);
                    subCategories[nextLevel].insert(subCategories[nextLevel].end(), nodeTypes.begin(), nodeTypes.end());
                }
            }
        }

        // Рисуем прямые элементы
        for (auto&& nodeTypeName : directNodes) {
            auto&& pMeta = SR_UTILS_NS::Factory::Instance().GetType(nodeTypeName);
            if (!pMeta) {
                continue;
            }

            auto&& displayName = pMeta->GetDisplayName();
            SR_UTILS_NS::StringAtom menuName = displayName.empty() ? nodeTypeName : displayName;

            if (!m_createNodeSearch.empty() && !SR_UTILS_NS::StringUtils::CheckSearchMatch(m_createNodeSearch, menuName.ToStringView())) {
                continue;
            }

            if (SR_GRAPH_GUI_NS::Immediate::MenuItem(menuName.c_str())) {
                OnNodeTypeSelected(nodeTypeName, popupPos);
            }
        }

        // Рисуем подменю
        for (auto&& [nextLevel, nodeTypes] : subCategories) {
            // Извлекаем имя следующего уровня
            std::string levelName = nextLevel;
            if (!prefix.empty()) {
                levelName = nextLevel.substr(prefix.size() + 1);
            }
            auto&& nextSlash = levelName.find('/');
            if (nextSlash != std::string::npos) {
                levelName = levelName.substr(0, nextSlash);
            }

            if (SR_GRAPH_GUI_NS::Immediate::BeginMenu(levelName.c_str())) {
                DrawNodeMenuRecursive(categories, nextLevel, popupPos);
                SR_GRAPH_GUI_NS::Immediate::EndMenu();
            }
        }
    }
}
