// 
// Created by innerviewer on 2025-04-06.
//

#ifndef SR_RENDER_SKIA_OVERLAY_H
#define SR_RENDER_SKIA_OVERLAY_H

#include <Graphics/Overlay/Overlay.h>

namespace SR_GRAPH_NS {
    class SkiaOverlay : public Overlay {
        using Super = Overlay;
    public:
        explicit SkiaOverlay(PipelinePtr pPipeline)
            : Super(std::move(pPipeline))
        { }

    public:
        SR_NODISCARD bool Init() override { return true; } // TODO: FIXME
        SR_NODISCARD bool IsUndockingActive() const override { return true; }  // TODO: FIXME
        SR_NODISCARD bool IsViewportsEnabled() const { return true; } // TODO: FIXME

        void Prepare() override { } // TODO: FIXME;
        void Destroy() override { } // TODO: FIXME;

    protected:
        virtual void ReloadFonts() { }; // TODO: FIXME;

    protected:
        //ImGuiContext* m_context = nullptr;

        SR_UTILS_NS::Path m_iniPathEditor;
        SR_UTILS_NS::Path m_iniPathWidgets;
    };
}

#endif //SR_RENDER_SKIA_OVERLAY_H
