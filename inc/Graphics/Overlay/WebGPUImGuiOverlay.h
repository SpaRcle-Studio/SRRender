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
#endif

namespace SR_GRAPH_NS {
    class WebGPUImGuiOverlay : public ImGuiOverlay {
        using Super = ImGuiOverlay;
    public:
        explicit WebGPUImGuiOverlay(PipelinePtr pPipeline)
            : Super(std::move(pPipeline))
        { }

        ~WebGPUImGuiOverlay() override {
            SRAssert2(m_wgpuRenderer == nullptr, "WebGPU ImGUI Overlay renderer is not destroyed");
        }

    public:
        SR_NODISCARD bool Init() override;
        SR_NODISCARD bool ReCreate() override;

        SR_NODISCARD std::string GetName() const override { return "WebGPU ImGUI"; }
        SR_NODISCARD bool IsDynamicRenderingEnabled() const override { return false; }
        SR_NODISCARD bool IsUndockingActive() const override { return false; }

        SR_NODISCARD void* GetTextureDescriptorSet(uint32_t textureId) override;
        void OnTextureFreed(uint32_t textureId) override;

        void ReloadFonts() override;
        void Destroy() override;

        bool BeginDraw() override;
        void EndDraw() override;

#if defined(SR_RENDER_USE_WEBGPU)
        void Render(WGPURenderPassEncoder pass);
#endif

    private:
        SR_GRAPH_GUI_NS::Immediate::PlatformBackend m_platformBackend = SR_GRAPH_GUI_NS::Immediate::PlatformBackend::None;
        SR_GRAPH_GUI_NS::Immediate::WebGPURendererHandle m_wgpuRenderer = nullptr;

#if defined(SR_RENDER_USE_WEBGPU)
        /// Cache: textureId -> WGPUTextureView (raw handle) last bound to ImGui.
        /// Used to detect when a view pointer changes (e.g. texture was re-uploaded).
        SR_HTYPES_NS::FlatHashMap<uint32_t, WGPUTextureView> m_textureViewCache;
#endif
    };
}

#endif //SR_ENGINE_GRAPHICS_WEBGPU_IMGUI_OVERLAY_H
