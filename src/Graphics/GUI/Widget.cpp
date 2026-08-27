//
// Created by Monika on 14.02.2022.
//

#include <Graphics/GUI/Widget.h>
#include <Graphics/GUI/WidgetManager.h>

#include <ImmediateGUI/GUI/ImmediateGUI.h>

#include <Codegen/Widget.generated.hpp>

namespace SR_GRAPH_GUI_NS {
    Widget::~Widget() {
        m_subWidgets.clear();
    }

    void Widget::DrawAsSubWindow() {
        SR_TRACY_ZONE;
        m_widgetFlags = WIDGET_FLAG_NONE;

        if (m_center) {
            auto&& position = SR_GRAPH_GUI_NS::Immediate::GetDisplaySize() * 0.5f;
            SR_GRAPH_GUI_NS::Immediate::SetNextWindowPos(position, SR_GRAPH_GUI_NS::Immediate::Condition::Always, SR_MATH_NS::FVector2(0.5f, 0.5f));
        }

        WindowFlags flags = m_windowFlags;

        if (IsFocused() || IsHovered()) {
            SR_MATH_NS::FVector4 color = SR_GRAPH_GUI_NS::Immediate::GetStyleColorVec4(SR_GRAPH_GUI_NS::Immediate::StyleColor::WindowBg);
            SR_GRAPH_GUI_NS::Immediate::PushStyleColor(SR_GRAPH_GUI_NS::Immediate::StyleColor::WindowBg, color + SR_MATH_NS::FVector4(0.02f, 0.02f, 0.02f, 0.f));
        }

        auto&& size = m_size.Contains(SR_INT32_MAX) ? SR_MATH_NS::FVector2(0, 0) : SR_MATH_NS::FVector2(m_size.x, m_size.y);
        if (SR_GRAPH_GUI_NS::Immediate::BeginChild(m_name.c_str(), size, ChildWindowFlags::None, flags)) {
            Draw();

            for (auto&& pWidget : m_subWidgets) {
                pWidget->DrawAsSubWindow();
            }
        }

        if (IsFocused() || IsHovered())
            SR_GRAPH_GUI_NS::Immediate::PopStyleColor();

        InternalCheckFocused();
        InternalCheckHovered();

        SR_GRAPH_GUI_NS::Immediate::EndChild();
    }

    void Widget::DrawWindow()  {
        SR_TRACY_ZONE;
        SR_TRACY_ZONE_TEXT(GetName());

        m_widgetFlags = WIDGET_FLAG_NONE;

        if (m_center) {
            auto&& position = SR_GRAPH_GUI_NS::Immediate::GetDisplaySize() * 0.5f;
            SR_GRAPH_GUI_NS::Immediate::SetNextWindowPos(position, SR_GRAPH_GUI_NS::Immediate::Condition::Always, SR_MATH_NS::FVector2(0.5f, 0.5f));
        }

        WindowFlags flags = m_windowFlags;

        if (IsFocused() || IsHovered()) {
            SR_MATH_NS::FVector4 color = SR_GRAPH_GUI_NS::Immediate::GetStyleColorVec4(SR_GRAPH_GUI_NS::Immediate::StyleColor::WindowBg);
            SR_GRAPH_GUI_NS::Immediate::PushStyleColor(SR_GRAPH_GUI_NS::Immediate::StyleColor::WindowBg, color + SR_MATH_NS::FVector4(0.02f, 0.02f, 0.02f, 0.f));
        }

        if (!m_size.Contains(SR_INT32_MAX)) {
            SR_GRAPH_GUI_NS::Immediate::SetNextWindowSize(SR_MATH_NS::FVector2(m_size.x, m_size.y));
            flags |= WindowFlags::NoResize;
        }

        auto&& pPreviousViewport = SR_GRAPH_GUI_NS::Immediate::GetWindowViewport();

        bool open = m_open;
        if (SR_GRAPH_GUI_NS::Immediate::Begin(m_name.c_str(), &open, flags)) {
            auto&& windowSize = SR_GRAPH_GUI_NS::Immediate::GetWindowSize();
            if (windowSize.x < 40.0f || windowSize.y < 40.0f) {
                if (!m_defaultSize.HasZero()) {
                    SR_GRAPH_GUI_NS::Immediate::SetWindowSize(SR_MATH_NS::FVector2(m_defaultSize.x, m_defaultSize.y));
                }
            }

            auto&& pCurrentViewport = SR_GRAPH_GUI_NS::Immediate::GetWindowViewport();

            if (pPreviousViewport != pCurrentViewport) {
                ViewportsTableManager::Instance().RegisterWidget(this, pCurrentViewport);
            }

            if (!open) {
                Close();
            }
            else {
                Draw();
            }
        }

        if (IsFocused() || IsHovered())
            SR_GRAPH_GUI_NS::Immediate::PopStyleColor();

        InternalCheckFocused();
        InternalCheckHovered();

        SR_GRAPH_GUI_NS::Immediate::End();
    }

    bool Widget::IsHoveredImpl() const {
        return SR_GRAPH_GUI_NS::Immediate::IsWindowHovered();
    }

    bool Widget::IsFocusedImpl() const {
        return SR_GRAPH_GUI_NS::Immediate::IsWindowFocused();
    }

    void Widget::TextCenter(const std::string &text) const {
        float font_size = SR_GRAPH_GUI_NS::Immediate::GetFontSize() * text.size() / 2;
        SR_GRAPH_GUI_NS::Immediate::SameLine(SR_GRAPH_GUI_NS::Immediate::GetWindowSize().x / 2 - font_size + (font_size / 2));
        SR_GRAPH_GUI_NS::Immediate::Text("%s", text.c_str());
    }

    void Widget::Focus() {
        if (!IsFocused())  {
            SR_GRAPH_GUI_NS::Immediate::SetWindowFocus(m_name.c_str());
        }
    }

    void Widget::Open() {
        if (!m_open)
            OnOpen();

        m_open = true;
    }

    void Widget::Close() {
        if (m_open)
            OnClose();

        m_open = false;
    }

    void Widget::InternalCheckFocused() {
        if (IsFocusedImpl() || m_widgetFlags & WIDGET_FLAG_FOCUSED) {
            m_internalFlags |= WIDGET_FLAG_FOCUSED;
        }
        else if (IsFocused()) {
            m_internalFlags ^= WIDGET_FLAG_FOCUSED;
        }
    }

    void Widget::InternalCheckHovered() {
        if (IsHoveredImpl() || m_widgetFlags & WIDGET_FLAG_HOVERED) {
            m_internalFlags |= WIDGET_FLAG_HOVERED;
        }
        else if (IsHovered()) {
            m_internalFlags ^= WIDGET_FLAG_HOVERED;
        }
    }

    void Widget::CheckFocused() {
        m_widgetFlags |= IsFocusedImpl() ? WIDGET_FLAG_FOCUSED : WIDGET_FLAG_NONE;
    }

    void Widget::CheckHovered() {
        m_widgetFlags |= IsHoveredImpl() ? WIDGET_FLAG_HOVERED : WIDGET_FLAG_NONE;
    }

    void Widget::SetManager(WidgetManager* manager) {
        m_manager = manager;
        for (auto&& pWidget : m_subWidgets) {
            pWidget->SetManager(manager);
        }
    }

    Widget::RenderScenePtr Widget::GetRenderScene() const {
        return m_manager->GetRenderScene();
    }

    Widget::ContextPtr Widget::GetContext() const {
        return m_manager->GetContext();
    }

    void Widget::ResetWeakStorage() {
        m_weakStorage.Clear();
    }

    void Widget::ResetStrongStorage() {
        m_strongStorage.Clear();
    }

    void Widget::AddSubWidget(Widget::Ptr pWidget) {
        m_subWidgets.emplace_back(pWidget);
        pWidget->SetManager(m_manager);
        pWidget->Init();
    }

    void Widget::OnKeyDown(const SR_UTILS_NS::KeyboardInputData* pData) {
        for (auto&& pWidget : m_subWidgets) {
            pWidget->OnKeyDown(pData);
        }
        InputHandler::OnKeyDown(pData);
    }

    void Widget::OnKeyUp(const SR_UTILS_NS::KeyboardInputData* pData) {
        for (auto&& pWidget : m_subWidgets) {
            pWidget->OnKeyUp(pData);
        }
        InputHandler::OnKeyDown(pData);
    }

    void Widget::OnKeyPress(const SR_UTILS_NS::KeyboardInputData* pData) {
        for (auto&& pWidget : m_subWidgets) {
            pWidget->OnKeyPress(pData);
        }
        InputHandler::OnKeyDown(pData);
    }
}