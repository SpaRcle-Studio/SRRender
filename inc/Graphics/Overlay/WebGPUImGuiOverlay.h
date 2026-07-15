//
// Created by Monika on 15.07.2026.
//

#ifndef SR_ENGINE_GRAPHICS_WEBGPU_IMGUI_OVERLAY_H
#define SR_ENGINE_GRAPHICS_WEBGPU_IMGUI_OVERLAY_H

#include <Graphics/Overlay/ImGuiOverlay.h>

#include <ImmediateGUI/Backend/PlatformBackend.h>
#include <ImmediateGUI/Backend/WebGPURenderer.h>

#if defined(SR_RENDER_USE_WEBGPU)
    #include <webgpu/webgpu.h>
#else
    typedef struct WGPURenderPassEncoderImpl* WGPURenderPassEncoder;
#endif

namespace SR_GRAPH_NS {
    class WebGPUImGuiOverlay : public ImGuiOverlay {
        using Super = ImGuiOverlay;
    public:
        explicit WebGPUImGuiOverlay(PipelinePtr pPipeline)
            : Super(std::move(pPipeline))
        { }

        ~WebGPUImGuiOverlay() override = default;

    public:
        SR_NODISCARD bool Init() override;
        SR_NODISCARD bool ReCreate() override;

        SR_NODISCARD std::string GetName() const override { return "WebGPU ImGUI"; }
        SR_NODISCARD bool IsDynamicRenderingEnabled() const override { return false; } /// No multi-viewport on Web/Emscripten for now.

        void ReloadFonts() override;

        void Destroy() override;

        bool BeginDraw() override;
        void EndDraw() override;

        void Render(WGPURenderPassEncoder pass);

    private:
        SR_GRAPH_GUI_NS::Immediate::PlatformBackend m_platformBackend = SR_GRAPH_GUI_NS::Immediate::PlatformBackend::None;
        SR_GRAPH_GUI_NS::Immediate::WebGPURendererHandle m_wgpuRenderer = nullptr;
    };
}

#endif // SR_ENGINE_GRAPHICS_WEBGPU_IMGUI_OVERLAY_H

