//
// Created by Monika on 10.02.2022.
//

#include <Graphics/Render/RenderScene.h>
#include <Graphics/Render/RenderContext.h>
#include <Graphics/GUI/WidgetManager.h>
#include <Graphics/GUI/Widget.h>

#include <ImmediateGUI/GUI/ImmediateGUI.h>

#include <Utils/Common/Features.h>
#include <Utils/Types/SafePtrLockGuard.h>
#include <Utils/TypeTraits/Factory.h>

namespace SR_GRAPH_NS::GUI {
    WidgetManager::WidgetManager()
        : SR_UTILS_NS::NonCopyable()
        , SR_UTILS_NS::InputHandler()
    {
        m_ignoreNonFocused = SR_UTILS_NS::Features::Instance().Enabled("InputIgnoreNonFocusedWidgets", true);
    }

    void WidgetManager::Draw() {
        SR_SCOPED_LOCK;

        for (auto&&[name, widget] : m_widgets) {
            if (widget->IsOpen()) {
                widget->DrawWindow();
            }
        }
    }

    WidgetManager::~WidgetManager() {
        m_widgets.clear();
    }

    void WidgetManager::OnKeyDown(const SR_UTILS_NS::KeyboardInputData* data) {
        SR_SCOPED_LOCK;

        for (auto&&[name, pWidget] : m_widgets) {
            if (pWidget->IsFocused() || !m_ignoreNonFocused)
                pWidget->OnKeyDown(data);
        }
    }

    void WidgetManager::OnKeyUp(const SR_UTILS_NS::KeyboardInputData* data) {
        SR_SCOPED_LOCK;

        for (auto&&[name, pWidget] : m_widgets) {
            if (pWidget->IsFocused() || !m_ignoreNonFocused)
                pWidget->OnKeyUp(data);
        }
    }

    void WidgetManager::OnKeyPress(const SR_UTILS_NS::KeyboardInputData *data) {
        SR_SCOPED_LOCK;

        for (auto&&[name, pWidget] : m_widgets) {
            if (pWidget->IsFocused() || !m_ignoreNonFocused)
                pWidget->OnKeyPress(data);
        }
    }

    void WidgetManager::OnMouseMove(const SR_UTILS_NS::MouseInputData *data) {
        SR_SCOPED_LOCK;

        for (auto&&[name, pWidget] : m_widgets) {
            if (pWidget->IsFocused() || !m_ignoreNonFocused)
                pWidget->OnMouseMove(data);
        }
    }

    void WidgetManager::OnMouseUp(const SR_UTILS_NS::MouseInputData *data) {
        SR_SCOPED_LOCK;

        for (auto&&[name, pWidget] : m_widgets) {
            if (pWidget->IsFocused() || !m_ignoreNonFocused)
                pWidget->OnMouseUp(data);
        }
    }

    void WidgetManager::OnMouseDown(const SR_UTILS_NS::MouseInputData *data) {
        SR_SCOPED_LOCK;

        for (auto&&[name, pWidget] : m_widgets) {
            if (pWidget->IsFocused() || !m_ignoreNonFocused)
                pWidget->OnMouseDown(data);
        }
    }

    void WidgetManager::OnMousePress(const SR_UTILS_NS::MouseInputData *data) {
        SR_SCOPED_LOCK;

        for (auto&&[name, pWidget] : m_widgets) {
            if (pWidget->IsFocused() || !m_ignoreNonFocused)
                pWidget->OnMousePress(data);
        }
    }

    void WidgetManager::SetRenderScene(const WidgetManager::RenderScenePtr& renderScene) {
        m_renderScene = renderScene;
    }

    WidgetManager::RenderScenePtr WidgetManager::GetRenderScene() const {
        return m_renderScene;
    }

    WidgetManager::ContextPtr WidgetManager::GetContext() const {
        return m_renderContext;
    }

    void WidgetManager::SetScene(const WidgetManager::ScenePtr &scene) {
        SR_LOCK_GUARD;

        for (auto&& [id, pWidget] : m_widgets) {
            pWidget->SetScene(scene);
        }
    }

    void WidgetManager::HideAll() {
        for (auto&& widget : ViewportsTableManager::Instance().GetViewportsTable()) {
            auto&& pPlatformHandle = SR_GRAPH_GUI_NS::Immediate::GetViewportPlatformHandle(widget.first);
            SR_PLATFORM_NS::ShowWindow(pPlatformHandle, SR_PLATFORM_NS::ShowWindowActionType::Hide);
        }
    }

    void WidgetManager::ShowAll() {
        for (auto&& widget : ViewportsTableManager::Instance().GetViewportsTable()) {
            auto&& pPlatformHandle = SR_GRAPH_GUI_NS::Immediate::GetViewportPlatformHandle(widget.first);
            SR_PLATFORM_NS::ShowWindow(pPlatformHandle, SR_PLATFORM_NS::ShowWindowActionType::Show);
        }
    }

    void WidgetManager::SetRenderContext(WidgetManager::ContextPtr pContext) {
        m_renderContext = pContext;
    }

    void WidgetManager::CloseAllWidgets() {
        for (auto& [id, widget] : m_widgets) {
            widget->Close();
        }
    }

    WidgetManager::WidgetPtr WidgetManager::GetWidget(SR_UTILS_NS::StringAtom name) const {
        if (auto&& pIt = m_widgets.find(name); pIt != m_widgets.end()) {
            return pIt->second;
        }

        return nullptr;
    }

    bool WidgetManager::Init() {
        auto&& factory = SR_UTILS_NS::Factory::Instance();
        auto&& widgets = factory.GetInheritances(SR_GRAPH_GUI_NS::Widget::GetClassStaticName());
        for (auto&& widget : widgets) {
            if (factory.IsAbstract(widget) || factory.GetType(widget)->IsHidden()) {
                continue;
            }
            if (auto&& pWidget = factory.Create<SR_GRAPH_GUI_NS::Widget>(widget)) {
                m_widgets[widget] = pWidget;
                pWidget->SetManager(this);
            }
        }

        for (auto&& [widgetName, pWidget] : m_widgets) {
            pWidget->Init();
        }

        return true;
    }

    Widget* ViewportsTableManager::GetWidgetByViewport(void *viewport) const {
        if (m_viewports.count(viewport) == 0) {
            return nullptr;
        }

        return m_viewports.at(viewport);
    }

    void ViewportsTableManager::RegisterWidget(Widget* widget, void* viewport) {
        m_viewports[viewport] = widget;
    }
}
