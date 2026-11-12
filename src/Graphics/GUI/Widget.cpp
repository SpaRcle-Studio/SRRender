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
            auto&& position = SR_IMMEDIATE_GUI_NS::GetDisplaySize() * 0.5f;
            SR_IMMEDIATE_GUI_NS::SetNextWindowPos(position, SR_IMMEDIATE_GUI_NS::Condition::Always, SR_MATH_NS::FVector2(0.5f, 0.5f));
        }

        WindowFlags flags = m_windowFlags;

        if (IsFocused() || IsHovered()) {
            SR_MATH_NS::FVector4 color = SR_IMMEDIATE_GUI_NS::GetStyleColorVec4(SR_IMMEDIATE_GUI_NS::StyleColor::WindowBg);
            SR_IMMEDIATE_GUI_NS::PushStyleColor(SR_IMMEDIATE_GUI_NS::StyleColor::WindowBg, color + SR_MATH_NS::FVector4(0.02f, 0.02f, 0.02f, 0.f));
        }

        auto&& size = m_size.Contains(SR_INT32_MAX) ? SR_MATH_NS::FVector2(0, 0) : SR_MATH_NS::FVector2(m_size.x, m_size.y);
        if (SR_IMMEDIATE_GUI_NS::BeginChild(m_name.c_str(), size, ChildWindowFlags::None, flags)) {
            Draw();

            for (auto&& pWidget : m_subWidgets) {
                pWidget->DrawAsSubWindow();
            }
        }

        if (IsFocused() || IsHovered())
            SR_IMMEDIATE_GUI_NS::PopStyleColor();

        InternalCheckFocused();
        InternalCheckHovered();

        SR_IMMEDIATE_GUI_NS::EndChild();
    }

    void Widget::DrawWindow()  {
        SR_TRACY_ZONE;
        SR_TRACY_ZONE_TEXT(GetName());

        m_widgetFlags = WIDGET_FLAG_NONE;

        if (m_center) {
            auto&& position = SR_IMMEDIATE_GUI_NS::GetDisplaySize() * 0.5f;
            SR_IMMEDIATE_GUI_NS::SetNextWindowPos(position, SR_IMMEDIATE_GUI_NS::Condition::Always, SR_MATH_NS::FVector2(0.5f, 0.5f));
        }

        WindowFlags flags = m_windowFlags;

        if (IsFocused() || IsHovered()) {
            SR_MATH_NS::FVector4 color = SR_IMMEDIATE_GUI_NS::GetStyleColorVec4(SR_IMMEDIATE_GUI_NS::StyleColor::WindowBg);
            SR_IMMEDIATE_GUI_NS::PushStyleColor(SR_IMMEDIATE_GUI_NS::StyleColor::WindowBg, color + SR_MATH_NS::FVector4(0.02f, 0.02f, 0.02f, 0.f));
        }

        if (!m_size.Contains(SR_INT32_MAX)) {
            SR_IMMEDIATE_GUI_NS::SetNextWindowSize(SR_MATH_NS::FVector2(m_size.x, m_size.y));
            flags |= WindowFlags::NoResize;
        }
        else if (!m_defaultSize.HasZero()) {
            SR_IMMEDIATE_GUI_NS::SetNextWindowSize(m_defaultSize.CastToFloat(), SR_IMMEDIATE_GUI_NS::Condition::FirstUseEver);
        }

        auto&& pPreviousViewport = SR_IMMEDIATE_GUI_NS::GetWindowViewport();

        bool open = m_open;
        if (SR_IMMEDIATE_GUI_NS::Begin(m_name.c_str(), &open, flags)) {
            auto&& windowSize = SR_IMMEDIATE_GUI_NS::GetWindowSize();
            if (windowSize.x < 40.0f || windowSize.y < 40.0f) {
                if (!m_defaultSize.HasZero()) {
                    SR_IMMEDIATE_GUI_NS::SetWindowSize(SR_MATH_NS::FVector2(m_defaultSize.x, m_defaultSize.y));
                }
            }

            auto&& pCurrentViewport = SR_IMMEDIATE_GUI_NS::GetWindowViewport();

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
            SR_IMMEDIATE_GUI_NS::PopStyleColor();

        InternalCheckFocused();
        InternalCheckHovered();

        SR_IMMEDIATE_GUI_NS::End();
    }

    bool Widget::IsHoveredImpl() const {
        return SR_IMMEDIATE_GUI_NS::IsWindowHovered();
    }

    bool Widget::IsFocusedImpl() const {
        return SR_IMMEDIATE_GUI_NS::IsWindowFocused();
    }

    void Widget::TextCenter(const std::string &text) const {
        float font_size = SR_IMMEDIATE_GUI_NS::GetFontSize() * text.size() / 2;
        SR_IMMEDIATE_GUI_NS::SameLine(SR_IMMEDIATE_GUI_NS::GetWindowSize().x / 2 - font_size + (font_size / 2));
        SR_IMMEDIATE_GUI_NS::Text("%s", text.c_str());
    }

    void Widget::Focus() {
        if (!IsFocused())  {
            SR_IMMEDIATE_GUI_NS::SetWindowFocus(m_name.c_str());
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
