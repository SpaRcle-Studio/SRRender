//
// Created by Monika on 01.03.2026.
//

#include <Graphics/Window/EmscriptenWindow.h>

#include <emscripten/html5.h>

#include <algorithm>

namespace SR_GRAPH_NS {
    bool EmscriptenWindow::IsFullScreen() {
        return EM_ASM_INT({ return document.fullscreenElement === document.querySelector('#srengine-canvas'); });
    }
    bool EmscriptenWindow::Initialize(
            const std::string& name, const SR_MATH_NS::IVector2 &position,
            const SR_MATH_NS::UVector2 &size, bool fullScreen, bool resizable
    ) {
        SR_LOG("EmscriptenWindow::Initialize() : initializing emscripten window...");
        m_isResizable = resizable;
        EM_ASM({ document.title = UTF8ToString($0); }, name.c_str());
        if (fullScreen) { emscripten_request_fullscreen(EMSCRIPTEN_CANVAS_ID, EM_TRUE); }

        double cssW = 0.0, cssH = 0.0;
        if (emscripten_get_element_css_size(EMSCRIPTEN_CANVAS_ID, &cssW, &cssH) == EMSCRIPTEN_RESULT_SUCCESS) {
            // Input/window coordinates are logical CSS pixels; the backing store follows DPR.
            const uint32_t pxW = static_cast<uint32_t>(std::max(1.0, cssW));
            const uint32_t pxH = static_cast<uint32_t>(std::max(1.0, cssH));

            m_size = { pxW, pxH };
        }
        else {
            // Fallback: reasonable defaults for browser canvas.
            m_size = { 800, 600 };
        }

        m_surfaceSize = m_size;

        m_isFocused = true;
        m_isValid = true;
        m_isInitialized = true;
        PollEvents();

        return true;
    }

    void EmscriptenWindow::PollEvents() {
        const bool focused = EM_ASM_INT({ return document.hasFocus() && !document.hidden; });
        if (focused != m_isFocused) {
            m_isFocused = focused;
            if (m_focusCallback) { m_focusCallback(this, focused); }
        }
        // Drive resize from the actual canvas CSS size (set by HTML/CSS).
        double cssW = 0.0, cssH = 0.0;
        if (emscripten_get_element_css_size(EMSCRIPTEN_CANVAS_ID, &cssW, &cssH) != EMSCRIPTEN_RESULT_SUCCESS) {
            return;
        }

        const uint32_t w = static_cast<uint32_t>(std::max(1.0, cssW));
        const uint32_t h = static_cast<uint32_t>(std::max(1.0, cssH));
        const double_t dpr = std::clamp(emscripten_get_device_pixel_ratio(), 1.0, 4.0);
        const uint32_t limit = static_cast<uint32_t>(EM_ASM_INT({ return Module['srMaxTextureDimension'] || 8192; }));
        const double_t scale = std::min(dpr, static_cast<double_t>(limit) / std::max(w, h));
        const SR_MATH_NS::UVector2 surface = {
            static_cast<uint32_t>(std::max(1.0, w * scale)), static_cast<uint32_t>(std::max(1.0, h * scale))};

        if (w == m_size.x && h == m_size.y && surface == m_surfaceSize) {
            return;
        }

        m_size = { w, h };
        m_surfaceSize = surface;

        emscripten_set_canvas_element_size(EMSCRIPTEN_CANVAS_ID, static_cast<int>(surface.x), static_cast<int>(surface.y));

        if (m_resizeCallback) {
            m_resizeCallback(this, static_cast<int32_t>(w), static_cast<int32_t>(h));
        }
    }
}
