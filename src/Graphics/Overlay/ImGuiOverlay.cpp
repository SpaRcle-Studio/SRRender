//
// Created by Monika on 15.09.2023.
//

#include <Graphics/Overlay/ImGuiOverlay.h>
#include <Graphics/GUI/Icons.h>
#include <Graphics/Pipeline/Pipeline.h>

#include <ImmediateGUI/GUI/ImmediateGUI.h>

#include <Utils/Common/StoreUtils.h>
#include <Utils/Common/Features.h>
#include <Utils/Common/CLIManager.h>
#include <Utils/FileSystem/FileSystem.h>
#include <Utils/Resources/ResourceManager.h>
#include <Utils/Platform/Platform.h>

#include <cstring>

namespace SR_GRAPH_NS {
    ImGuiOverlay::ImGuiOverlay(Overlay::PipelinePtr pPipeline)
        : Super(std::move(pPipeline))
    { }

    bool ImGuiOverlay::Init() {
        SR_GRAPH("ImGuiOverlay::Init() : initializing ImGui...");

        auto&& resourcesManager = SR_UTILS_NS::ResourceManager::Instance();

        static std::string_view editorConfigPath = "Editor/Configs/ImGuiEditor.config";
        static std::string_view widgetsConfigPath = "Editor/Configs/EditorWidgets.xml";

        m_iniPathEditor = resourcesManager.GetCachePath().Concat(editorConfigPath);
        m_iniPathWidgets = resourcesManager.GetCachePath().Concat(widgetsConfigPath);

        if (!m_iniPathEditor.Exists()) {
            SR_UTILS_NS::Path engineEditorConfigPath = resourcesManager.GetEngineCachePath().Concat(editorConfigPath);
            if (engineEditorConfigPath.Exists()) {
                SR_PLATFORM_NS::Copy(engineEditorConfigPath, m_iniPathEditor);
            }
            else {
                SR_UTILS_NS::Platform::Copy(resourcesManager.GetResPath().Concat(editorConfigPath), m_iniPathEditor);
            }
        }

        if (!m_iniPathWidgets.Exists()) {
            SR_UTILS_NS::Path engineWidgetsConfigPath = resourcesManager.GetEngineCachePath().Concat(widgetsConfigPath);
            if (engineWidgetsConfigPath.Exists()) {
                SR_PLATFORM_NS::Copy(engineWidgetsConfigPath, m_iniPathWidgets);
            }
            else {
                SR_UTILS_NS::Platform::Copy(resourcesManager.GetResPath().Concat(widgetsConfigPath), m_iniPathWidgets);
            }
        }

        SR_GRAPH_GUI_NS::Immediate::ImmediateGUICreateContext createContext;
        createContext.iniPath = m_iniPathEditor;
        createContext.viewportsEnabled = SR_UTILS_NS::Features::Instance().Enabled("Undocking", false);
        if (!IsDynamicRenderingEnabled()) {
            SR_WARN("ImGuiOverlay::Init() : dynamic rendering is disabled! Undocking is not available!");
            createContext.viewportsEnabled = false;
        }
        m_viewportsEnabled = createContext.viewportsEnabled;

        m_context = SR_GRAPH_GUI_NS::Immediate::CreateContext(createContext);

        ReloadFonts();

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
            SR_GRAPH("ImGuiOverlay::Destroy() : destroying ImGui context...");
            SR_GRAPH_GUI_NS::Immediate::DestroyContext(m_context);
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

        SR_GRAPH_GUI_NS::Immediate::ClearFonts();

        static const uint32_t mainFontRanges[] = {
            0x0020, 0x00FF, /// Basic Latin + Latin Supplement
            //0x0400, 0x044F, /// Cyrillic
            0x0400, 0x04FF, /// Cyrillic
            0,
        };
        static const uint32_t ranges[] = { 0x26A0, 0x26A0, 0 };
        static const uint32_t icon_ranges[] = { SR_ICON_MIN, SR_ICON_MAX, 0 };

        SR_UTILS_NS::String fontData;

        auto&& rm = SR_UTILS_NS::ResourceManager::Instance();

        auto loadFontOwnedByAtlas = [&](const SR_UTILS_NS::Path& fontPath, float size, const SR_GRAPH_GUI_NS::Immediate::ImmediateGUIFontConfig& cfg, const uint32_t* glyphRanges) -> void* {
            if (!fontPath.Exists()) {
                SR_ERROR("ImGuiOverlay::ReloadFonts() : file not found!\n\tPath: " + fontPath.ToString());
                return nullptr;
            }

            if (!SR_UTILS_NS::FileSystem::ReadFile(fontPath, fontData) || fontData.empty()) {
                SR_ERROR("ImGuiOverlay::ReloadFonts() : failed to read font data!\n\tPath: " + fontPath.ToString());
                return nullptr;
            }

            void* owned = SRMalloc(fontData.size());
            if (!owned) {
                SR_ERROR("ImGuiOverlay::ReloadFonts() : failed to allocate font memory!\n\tPath: " + fontPath.ToString());
                return nullptr;
            }
            memcpy(owned, fontData.data(), fontData.size());

            SR_GRAPH_GUI_NS::Immediate::ImmediateGUIFontConfig config = cfg;
            config.fontDataOwnedByAtlas = true; // ImGui will free via allocator we set in CreateContext()
            return SR_GRAPH_GUI_NS::Immediate::AddFontFromMemoryTTF(owned, static_cast<int>(fontData.size()), size, config, glyphRanges);
        };

        /// Main font
        {
            SR_TRACY_ZONE_N("Load main font");
            auto&& fontPath = rm.GetResPath().Concat("Engine/Fonts/tahoma.ttf");
            SR_GRAPH("ImGuiOverlay::ReloadFonts() : load editor font...\n\tPath: " + fontPath.ToString());

            SR_GRAPH_GUI_NS::Immediate::ImmediateGUIFontConfig config;
            m_smallFont = loadFontOwnedByAtlas(fontPath, m_fontSize * 0.75f, config, mainFontRanges);
            m_mainFont  = loadFontOwnedByAtlas(fontPath, m_fontSize, config, mainFontRanges);
        }

        /// Warning font - for icons like "⚠" merge with main font
        {
            SR_TRACY_ZONE_N("Load warning font");
            auto&& fontPath = rm.GetResPath().Concat("Engine/Fonts/seguisym.ttf");
            SR_GRAPH("ImGuiOverlay::ReloadFonts() : load warning font...\n\tPath: " + fontPath.ToString());
            SR_GRAPH_GUI_NS::Immediate::ImmediateGUIFontConfig config;
            config.mergeMode = true;
            loadFontOwnedByAtlas(fontPath, m_fontSize, config, ranges);
        }

        /// Icons font
        {
            SR_TRACY_ZONE_N("Load icons font");
            auto&& iconsFont = rm.GetResPath().Concat("Engine/Fonts/fa-solid-900.ttf");

            SR_GRAPH("ImGuiOverlay::ReloadFonts() : load icon font...\n\tPath: " + iconsFont.ToString());
            SR_GRAPH_GUI_NS::Immediate::ImmediateGUIFontConfig config;
            config.mergeMode = false;
            config.glyphMinAdvanceX = 13.0f;
            m_iconFont = loadFontOwnedByAtlas(iconsFont, m_iconFontSize, config, icon_ranges);
        }

        SR_GRAPH("ImGuiOverlay::ReloadFonts() : build fonts...");
        if (!SR_GRAPH_GUI_NS::Immediate::BuildFonts(m_mainFont)) {
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
            return SR_GRAPH_GUI_NS::Immediate::GetViewportCount(m_context) > 1;
        }

        return false;
    }

    bool ImGuiOverlay::IsViewportsEnabled() const {
        return m_viewportsEnabled;
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