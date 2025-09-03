//
// Created by Monika on 15.09.2023.
//

#include <Utils/Common/StoreUtils.h>

#include <Graphics/Overlay/ImGuiOverlay.h>
#include <Graphics/GUI/Editor/Theme.h>
#include <Graphics/GUI/Icons.h>

#include <Graphics/GUI/ImGUI.h>

namespace SR_GRAPH_NS {
    bool ImGuiOverlay::Init() {
        SR_GRAPH("ImGuiOverlay::Init() : initializing ImGui...");

        m_context = ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO(); (void)io;

        ReloadFonts();

        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigDockingWithShift = true;
        io.ConfigWindowsResizeFromEdges = true;
        io.ConfigViewportsNoDecoration = true;
        io.ConfigWindowsMoveFromTitleBarOnly = true;

        if (SR_UTILS_NS::Features::Instance().Enabled("Undocking", false)) {
            if (IsDynamicRenderingEnabled()) {
                io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            }
            else {
                SR_WARN("ImGuiOverlay::Init() : dynamic rendering is disabled! Undocking is not available!");
            }
        }

        if (auto&& pTheme = SR_GRAPH_GUI_NS::Theme::Load("Engine/Configs/Themes/Dark.xml")) {
            pTheme->Apply();
            delete pTheme;
        }
        else {
            SR_ERROR("Engine::InitializeRender() : failed to load theme!");
        }

        auto&& resourcesManager = SR_UTILS_NS::ResourceManager::Instance();

        m_iniPathEditor = resourcesManager.GetCachePath().Concat("Editor/Configs/ImGuiEditor.config");
        m_iniPathWidgets = resourcesManager.GetCachePath().Concat("Editor/Configs/EditorWidgets.xml");

        if (!m_iniPathEditor.Exists()) {
            m_iniPathEditor.Create();
            SR_UTILS_NS::Platform::Copy(resourcesManager.GetResPath().Concat("Editor/Configs/ImGuiEditor.config"), m_iniPathEditor);
        }

        if (!m_iniPathWidgets.Exists()) {
            m_iniPathWidgets.Create();
            SR_UTILS_NS::Platform::Copy(resourcesManager.GetResPath().Concat("Editor/Configs/EditorWidgets.xml"), m_iniPathWidgets);
        }

        io.IniFilename = m_iniPathEditor.CStr();

        return true;
    }

    void ImGuiOverlay::Prepare() {
        SR_TRACY_ZONE;

        Super::Prepare();

        if (!IsEnabled()) {
            return;
        }

        const bool isFontChanged =
            m_fontSize != SR_UTILS_NS::StoreUtils::User::GetFloat("ImGuiFontSize", m_fontSize) ||
            m_iconFontSize != SR_UTILS_NS::StoreUtils::User::GetFloat("ImGuiIconFontSize", m_iconFontSize);

        if (isFontChanged) {
            ReloadFonts();
            GetPipeline()->SetDirty(true);
        }
    }

    void ImGuiOverlay::Destroy() {
        if (m_context) {
            ImGui::DestroyContext(((ImGuiContext*)m_context));
            m_context = nullptr;
        }
    }

    void ImGuiOverlay::ReloadFonts() {
        SR_TRACY_ZONE;

        SR_INFO("ImGuiOverlay::ReloadFonts() : reloading fonts...");

        m_fontSize = SR_UTILS_NS::StoreUtils::User::GetFloat("ImGuiFontSize", m_fontSize);
        m_iconFontSize = SR_UTILS_NS::StoreUtils::User::GetFloat("ImGuiIconFontSize", m_iconFontSize);

        SR_UTILS_NS::StoreUtils::User::SetFloat("ImGuiFontSize", m_fontSize);
        SR_UTILS_NS::StoreUtils::User::SetFloat("ImGuiIconFontSize", m_iconFontSize);

        ImGuiIO& io = ImGui::GetIO(); (void)io;

        io.Fonts->Clear();

        auto&& fontPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat("Engine/Fonts/tahoma.ttf");

        static const ImWchar ranges[] = {
            0x0020, 0x00FF, /// Basic Latin + Latin Supplement
            0x0400, 0x044F, /// Cyrillic
            0,
        };

        SR_GRAPH("ImGuiOverlay::ReloadFonts() : load editor font...\n\tPath: " + fontPath.ToString());
        if (fontPath.Exists()) {
            ImFontConfig font_config;
            font_config.OversampleH = 1; /// Or 2 is the same
            font_config.OversampleV = 1;
            font_config.PixelSnapH = true;
            m_mainFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), m_fontSize, nullptr, ranges);
            m_smallFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), m_fontSize * 0.75f, nullptr, ranges);
        }
        else {
            SR_ERROR("ImGuiOverlay::ReloadFonts() : file not found!\n\tPath: " + fontPath.ToString());
        }

        auto&& iconsFont = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat("Engine/Fonts/fa-solid-900.ttf");

        SR_GRAPH("ImGuiOverlay::ReloadFonts() : load icon font...\n\tPath: " + iconsFont.ToString());
        if (iconsFont.Exists()) {
            ImFontConfig config;
            config.MergeMode = false;
            config.GlyphMinAdvanceX = 13.0f;
            static const ImWchar icon_ranges[] = { SR_ICON_MIN, SR_ICON_MAX, 0 };
            m_iconFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(iconsFont.CStr(), m_iconFontSize, &config, icon_ranges);
        }
        else {
            SR_ERROR("ImGuiOverlay::ReloadFonts() : file not found! \n\tPath: " + iconsFont.ToString());
        }

        SR_GRAPH("ImGuiOverlay::ReloadFonts() : build fonts...");
        if (!io.Fonts->Build()) {
            SR_ERROR("ImGuiOverlay::ReloadFonts() : failed to build fonts!");
            return;
        }

        if (m_mainFont && m_iconFont) {
            SR_GRAPH("ImGuiOverlay::ReloadFonts() : fonts loaded successfully!");
        }
        else {
            SR_ERROR("ImGuiOverlay::ReloadFonts() : failed to load fonts!");
        }
    }

    bool ImGuiOverlay::IsUndockingActive() const {
        if (m_context && IsViewportsEnabled()) {
            return ((ImGuiContext*)m_context)->Viewports.size() > 1;
        }

        return false;
    }

    bool ImGuiOverlay::IsViewportsEnabled() const {
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            return true;
        }

        return false;
    }
}