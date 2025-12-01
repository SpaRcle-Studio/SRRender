//
// Created by Monika on 01.12.2025.
//

#include <Graphics/Window/HeadlessWindow.h>

namespace SR_GRAPH_NS {
    bool HeadlessWindow::Initialize(
            const std::string& name, const SR_MATH_NS::IVector2 &position,
            const SR_MATH_NS::UVector2 &size, bool fullScreen, bool resizable
    ) {
        SR_LOG("HeadlessWindow::Initialize() : initializing headless window...");

        m_size = { 128, 128 };
        m_surfaceSize = m_size;

        m_isValid = true;
        m_isInitialized = true;

        return true;
    }
}
