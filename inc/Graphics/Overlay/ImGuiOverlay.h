//
// Created by Monika on 15.09.2023.
//

#ifndef SR_ENGINE_GRAPHICS_IMGUI_OVERLAY_H
#define SR_ENGINE_GRAPHICS_IMGUI_OVERLAY_H

#include <Graphics/Overlay/Overlay.h>

#include <Utils/FileSystem/Path.h>

namespace SR_GRAPH_NS {
    class SR_GRAPHICS_DLL_API ImGuiOverlay : public Overlay {
        using Super = Overlay;
    public:
        explicit ImGuiOverlay(PipelinePtr pPipeline);

    public:
        SR_NODISCARD bool Init() override;
        SR_NODISCARD bool IsUndockingActive() const override;
        SR_NODISCARD bool IsViewportsEnabled() const;

        SR_NODISCARD void* GetIconFont() const;
        SR_NODISCARD void* GetMainFont() const;
        SR_NODISCARD void* GetSmallFont() const;

        void Prepare() override;
        void Destroy() override;

    protected:
        virtual void ReloadFonts();

    protected:
        void* m_context = nullptr;
        void* m_mainFont = nullptr;
        void* m_smallFont = nullptr;
        void* m_iconFont = nullptr;

        float_t m_fontSize = 12.0f;
        float_t m_iconFontSize = 40.0f;

        SR_UTILS_NS::Path m_iniPathEditor;
        SR_UTILS_NS::Path m_iniPathWidgets;

    };
}

#endif //SR_ENGINE_GRAPHICS_IMGUI_OVERLAY_H
