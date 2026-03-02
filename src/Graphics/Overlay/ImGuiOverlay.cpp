//
// Created by Monika on 15.09.2023.
//

#include <Graphics/Overlay/ImGuiOverlay.h>
#include <Graphics/GUI/Editor/Theme.h>
#include <Graphics/GUI/Icons.h>
#include <Graphics/GUI/ImGUI.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <Utils/Common/StoreUtils.h>
#include <Utils/Common/Features.h>
#include <Utils/Resources/ResourceManager.h>

namespace SR_GRAPH_NS {
    ImGuiOverlay::ImGuiOverlay(Overlay::PipelinePtr pPipeline)
        : Super(std::move(pPipeline))
    { }

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

        static const SR_UTILS_NS::StringAtom fontSizeKey = "ImGuiFontSize";
        static const SR_UTILS_NS::StringAtom iconFontSizeKey = "ImGuiIconFontSize";

        const bool isFontChanged =
            m_fontSize != SR_UTILS_NS::StoreUtils::User::GetFloat(fontSizeKey, m_fontSize) ||
            m_iconFontSize != SR_UTILS_NS::StoreUtils::User::GetFloat(iconFontSizeKey, m_iconFontSize);

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

        static const ImWchar mainFontRanges[] = {
            0x0020, 0x00FF, /// Basic Latin + Latin Supplement
            //0x0400, 0x044F, /// Cyrillic
            0x0400, 0x04FF, /// Cyrillic
            0,
        };

        std::string fontData;

        /// Main font
        {
            SR_TRACY_ZONE_N("Load main font");
            auto&& fontPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat("Engine/Fonts/tahoma.ttf");
            SR_GRAPH("ImGuiOverlay::ReloadFonts() : load editor font...\n\tPath: " + fontPath.ToString());

            if (fontPath.Exists()) {
                ImFontConfig font_config;
                font_config.OversampleH = 1; /// Or 2 is the same
                font_config.OversampleV = 1;
                font_config.PixelSnapH = true;
                font_config.MergeMode = false;

                if (SR_UTILS_NS::FileSystem::ReadFile(fontPath, fontData)) {
                    ImFontConfig config;
                    config.FontDataOwnedByAtlas = false;

                    m_smallFont = io.Fonts->AddFontFromMemoryTTF((void*)fontData.c_str(), fontData.size(), m_fontSize * 0.75f, &config, mainFontRanges);
                    m_mainFont = io.Fonts->AddFontFromMemoryTTF((void*)fontData.c_str(), fontData.size(), m_fontSize, &config, mainFontRanges);
                }
                else {
                    SR_ERROR("ImGuiOverlay::ReloadFonts() : failed to read font data!\n\tPath: " + fontPath.ToString());
                }
            }
            else {
                SR_ERROR("ImGuiOverlay::ReloadFonts() : file not found!\n\tPath: " + fontPath.ToString());
            }
        }

        /// Warning font - for icons like "⚠" merge with main font
        {
            SR_TRACY_ZONE_N("Load warning font");
            auto&& fontPath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat("Engine/Fonts/seguisym.ttf");
            SR_GRAPH("ImGuiOverlay::ReloadFonts() : load warning font...\n\tPath: " + fontPath.ToString());
            ImFontConfig cfg;
            cfg.MergeMode = true;
            static const ImWchar ranges[] = { 0x26A0, 0x26A0, 0 };
            io.Fonts->AddFontFromFileTTF(fontPath.CStr(), m_fontSize, &cfg, ranges);
        }

        /// Icons font
        {
            SR_TRACY_ZONE_N("Load icons font");
            auto&& iconsFont = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat("Engine/Fonts/fa-solid-900.ttf");

            SR_GRAPH("ImGuiOverlay::ReloadFonts() : load icon font...\n\tPath: " + iconsFont.ToString());
            if (iconsFont.Exists()) {
                ImFontConfig config;
                config.MergeMode = false;
                config.GlyphMinAdvanceX = 13.0f;
                config.FontDataOwnedByAtlas = false;
                static const ImWchar icon_ranges[] = { SR_ICON_MIN, SR_ICON_MAX, 0 };
                if (SR_UTILS_NS::FileSystem::ReadFile(iconsFont, fontData)) {
                    m_iconFont = io.Fonts->AddFontFromMemoryTTF((void*)fontData.c_str(), fontData.size(), m_iconFontSize, &config, icon_ranges);
                }
            }
            else {
                SR_ERROR("ImGuiOverlay::ReloadFonts() : file not found! \n\tPath: " + iconsFont.ToString());
            }
        }

        io.FontDefault = static_cast<ImFont*>(m_mainFont);

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

    void* ImGuiOverlay::GetIconFont() const {
        return m_iconFont;
    }

    void* ImGuiOverlay::GetMainFont() const {
        return m_mainFont;
    }

    void* ImGuiOverlay::GetSmallFont() const {
        return m_smallFont;
    }

}