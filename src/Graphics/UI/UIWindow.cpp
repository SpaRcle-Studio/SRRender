//
// Created by Monika on 02.08.2024.
//

#include <Graphics/UI/UIWindow.h>

namespace SR_GRAPH_UI_NS {
    bool UIWindow::InitializeEntity() noexcept {
        GetComponentProperties().AddStandardProperty<bool>("Resizable")
            .SetGetter([this](void* pData) { *static_cast<bool*>(pData) = IsResizable(); })
            .SetSetter([this](void* pData) { SetResizable(*static_cast<bool*>(pData)); });

        GetComponentProperties().AddStandardProperty<bool>("Movable")
            .SetGetter([this](void* pData) { *static_cast<bool*>(pData) = IsMovable(); })
            .SetSetter([this](void* pData) { SetMovable(*static_cast<bool*>(pData)); });

        GetComponentProperties().AddStandardProperty<bool>("Closable")
            .SetGetter([this](void* pData) { *static_cast<bool*>(pData) = IsClosable(); })
            .SetSetter([this](void* pData) { SetClosable(*static_cast<bool*>(pData)); });

        GetComponentProperties().AddStandardProperty<bool>("Minimizable")
            .SetGetter([this](void* pData) { *static_cast<bool*>(pData) = IsMinimizable(); })
            .SetSetter([this](void* pData) { SetMinimizable(*static_cast<bool*>(pData)); });

        GetComponentProperties().AddStandardProperty<bool>("Maximizable")
            .SetGetter([this](void* pData) { *static_cast<bool*>(pData) = IsMaximizable(); })
            .SetSetter([this](void* pData) { SetMaximizable(*static_cast<bool*>(pData)); });

        GetComponentProperties().AddStandardProperty<bool>("Dockable")
            .SetGetter([this](void* pData) { *static_cast<bool*>(pData) = IsDockable(); })
            .SetSetter([this](void* pData) { SetDockable(*static_cast<bool*>(pData)); });

        GetComponentProperties().AddStandardProperty<SR_MATH_NS::UVector2>("Window size")
            .SetGetter([this](void* pData) { *static_cast<SR_MATH_NS::UVector2*>(pData) = m_windowSize; })
            .SetSetter([this](void* pData) { SetWindowSize(*static_cast<SR_MATH_NS::UVector2*>(pData)); });

        return true;
    }

    void UIWindow::SetResizable(bool value) noexcept {
    }

    void UIWindow::SetMovable(bool value) noexcept {
    }

    void UIWindow::SetClosable(bool value) noexcept {
    }

    void UIWindow::SetMinimizable(bool value) noexcept {
    }

    void UIWindow::SetMaximizable(bool value) noexcept {
    }

    void UIWindow::SetDockable(bool value) noexcept {
    }

    void UIWindow::SetWindowSize(const SR_MATH_NS::UVector2& size) noexcept {
        m_windowSize = size;

        if (!GetGameObject()) {
            return;
        }

        m_content = GetGameObject()->Find("[Content]");
        m_titleBar = GetGameObject()->Find("[TitleBar]");

        if (!m_content || !m_titleBar) {
            return;
        }

    }
}

