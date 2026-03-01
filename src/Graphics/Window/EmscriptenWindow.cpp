//
// Created by Monika on 01.03.2026.
//

#include <Graphics/Window/EmscriptenWindow.h>

namespace SR_GRAPH_NS {
    bool EmscriptenWindow::Initialize(
            const std::string& name, const SR_MATH_NS::IVector2 &position,
            const SR_MATH_NS::UVector2 &size, bool fullScreen, bool resizable
    ) {
        SR_LOG("EmscriptenWindow::Initialize() : initializing headless window...");

        m_size = { 128, 128 };
        m_surfaceSize = m_size;

        m_isValid = true;
        m_isInitialized = true;

        return true;
    }
}
