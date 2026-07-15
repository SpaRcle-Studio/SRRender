//
// Created by Monika on 15.07.2026.
//

#include <Graphics/Overlay/WebGPUImGuiOverlay.h>
#include <Graphics/Pipeline/WebGPU/WebGPUPipeline.h>
#include <Graphics/Window/Window.h>

#include <ImmediateGUI/GUI/ImmediateGUI.h>

namespace SR_GRAPH_NS {
    bool WebGPUImGuiOverlay::Init() {
        SR_TRACY_ZONE;

        if (!Super::Init()) {
            return false;
        }

    #if defined(SR_EMSCRIPTEN)
        m_platformBackend = SR_GRAPH_GUI_NS::Immediate::PlatformBackend::Emscripten;
    #else
        m_platformBackend = SR_GRAPH_GUI_NS::Immediate::PlatformBackend::None;
    #endif

        auto&& pWindow = m_pipeline->GetWindow();

        SR_GRAPH_GUI_NS::Immediate::PlatformInitInfo platformInit = { };
        platformInit.backend = m_platformBackend;
        platformInit.window = pWindow ? pWindow->GetHandle() : nullptr;

        if (!SR_GRAPH_GUI_NS::Immediate::PlatformInit(platformInit)) {
            SR_ERROR("WebGPUImGuiOverlay::Init() : failed to initialize platform backend!");
            return false;
        }

        auto&& pWebGPUPipeline = SR_UTILS_NS::DynamicPointerCast<WebGPUPipeline>(m_pipeline);
        if (!pWebGPUPipeline) {
            SR_ERROR("WebGPUImGuiOverlay::Init() : pipeline is not WebGPUPipeline!");
            return false;
        }

        SR_GRAPH_GUI_NS::Immediate::WebGPURendererCreateInfo wgpuInfo;
        wgpuInfo.device = pWebGPUPipeline->GetWGPUDevice();
        wgpuInfo.numFramesInFlight = pWebGPUPipeline->GetFramesInFlightCount();
        wgpuInfo.renderTargetFormat = pWebGPUPipeline->GetSurfaceFormat();
        wgpuInfo.depthStencilFormat = WGPUTextureFormat_Undefined;

        m_wgpuRenderer = SR_GRAPH_GUI_NS::Immediate::WebGPUCreateRenderer(wgpuInfo);
        if (!m_wgpuRenderer) {
            SR_ERROR("WebGPUImGuiOverlay::Init() : failed to create WebGPU imgui renderer!");
            return false;
        }

        if (!ReCreate()) {
            SR_ERROR("WebGPUImGuiOverlay::Init() : failed to re-create!");
            return false;
        }

        m_initialized = true;
        return true;
    }

    void WebGPUImGuiOverlay::Destroy() {
        if (m_wgpuRenderer) {
            SR_GRAPH_GUI_NS::Immediate::WebGPUDestroyRenderer(m_wgpuRenderer);
            m_wgpuRenderer = nullptr;
        }

        if (m_initialized) {
            SR_GRAPH_GUI_NS::Immediate::PlatformShutdown(m_platformBackend);
        }

        m_initialized = false;

        ImGuiOverlay::Destroy();
    }

    bool WebGPUImGuiOverlay::BeginDraw() {
        SR_TRACY_ZONE;

        if (!m_context || !m_wgpuRenderer) {
            return false;
        }

        SR_GRAPH_GUI_NS::Immediate::WebGPUNewFrame(m_wgpuRenderer);

        SR_GRAPH_GUI_NS::Immediate::PlatformNewFrame(m_platformBackend);
        SR_GRAPH_GUI_NS::Immediate::NewFrame();

        return true;
    }

    void WebGPUImGuiOverlay::EndDraw() {
        SR_TRACY_ZONE;

        SR_GRAPH_GUI_NS::Immediate::Render();

        if (IsViewportsEnabled()) {
            SR_GRAPH_GUI_NS::Immediate::UpdatePlatformWindows();
            SR_GRAPH_GUI_NS::Immediate::RenderPlatformWindowsDefault();
        }
    }

    bool WebGPUImGuiOverlay::ReCreate() {
        m_surfaceDirty = false;
        return true;
    }

    void WebGPUImGuiOverlay::ReloadFonts() {
        Super::ReloadFonts();
        SR_GRAPH_GUI_NS::Immediate::WebGPUReloadFonts(m_wgpuRenderer);
    }

    void WebGPUImGuiOverlay::Render(WGPURenderPassEncoder pass) {
        SR_TRACY_ZONE;
        SR_GRAPH_GUI_NS::Immediate::WebGPURenderDrawData(m_wgpuRenderer, pass);
    }
}

