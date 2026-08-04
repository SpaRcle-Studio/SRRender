//
// Created by Monika on 01.03.2026.
//

#include <Graphics/Window/EmscriptenWindow.h>

#include <emscripten/html5.h>

#include <algorithm>

namespace SR_GRAPH_NS {
    bool EmscriptenWindow::Initialize(
            const std::string& name, const SR_MATH_NS::IVector2 &position,
            const SR_MATH_NS::UVector2 &size, bool fullScreen, bool resizable
    ) {
        SR_LOG("EmscriptenWindow::Initialize() : initializing emscripten window...");

        double cssW = 0.0, cssH = 0.0;
        if (emscripten_get_element_css_size(EMSCRIPTEN_CANVAS_ID, &cssW, &cssH) == EMSCRIPTEN_RESULT_SUCCESS) {
            // Keep window size in CSS pixels (surface is configured in CSS pixels too).
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

        return true;
    }

    void EmscriptenWindow::PollEvents() {
        // Drive resize from the actual canvas CSS size (set by HTML/CSS).
        double cssW = 0.0, cssH = 0.0;
        if (emscripten_get_element_css_size(EMSCRIPTEN_CANVAS_ID, &cssW, &cssH) != EMSCRIPTEN_RESULT_SUCCESS) {
            return;
        }

        const uint32_t w = static_cast<uint32_t>(std::max(1.0, cssW));
        const uint32_t h = static_cast<uint32_t>(std::max(1.0, cssH));

        if (w == m_size.x && h == m_size.y) {
            return;
        }

        m_size = { w, h };
        m_surfaceSize = m_size;

        // Keep canvas backing store in sync with CSS pixels (no DPR scaling).
        emscripten_set_canvas_element_size(EMSCRIPTEN_CANVAS_ID, static_cast<int>(w), static_cast<int>(h));

        if (m_resizeCallback) {
            m_resizeCallback(this, static_cast<int32_t>(w), static_cast<int32_t>(h));
        }
    }
}
