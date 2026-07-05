//
// Created by Monika on 10.02.2022.
//

#ifndef SR_ENGINE_WIDGETMANAGER_H
#define SR_ENGINE_WIDGETMANAGER_H

#include <Graphics/macros.h>

#include <Utils/Types/Thread.h>
#include <Utils/Types/SharedPtr.h>
#include <Utils/Input/InputHandler.h>
#include <Utils/Input/InputSystem.h>

namespace SR_WORLD_NS {
    class Scene;
}

namespace SR_GRAPH_NS {
    class RenderScene;
    class RenderContext;
}

namespace SR_GRAPH_NS::GUI {
    class Widget;

    typedef SR_HTYPES_NS::FlatHashMap<std::string, Widget*> Widgets;
    typedef SR_HTYPES_NS::FlatHashMap<void*, Widget*> ViewportsTable;

    class WidgetManager : public SR_UTILS_NS::NonCopyable, public SR_UTILS_NS::InputHandler {
        using RenderScenePtr = SR_HTYPES_NS::SharedPtr<RenderScene>;
        using ContextPtr = RenderContext*;
        using ScenePtr = SR_HTYPES_NS::SharedPtr<SR_WORLD_NS::Scene>;
        using WidgetPtr = SR_HTYPES_NS::SharedPtr<SR_GRAPH_GUI_NS::Widget>;
        using Widgets = SR_HTYPES_NS::FlatHashMap<SR_UTILS_NS::StringAtom, WidgetPtr>;
    public:
        WidgetManager();
        ~WidgetManager() override;

        virtual void Draw();

        virtual bool Init();
        virtual void DeInit() { }

        void HideAll();
        void ShowAll();

        void CloseAllWidgets();

        SR_NODISCARD WidgetPtr GetWidget(SR_UTILS_NS::StringAtom name) const;

        template<typename T> SR_HTYPES_NS::SharedPtr<T> GetWidget() {
            SR_UTILS_NS::StringAtom widgetName = T::GetMetaStatic()->GetFactoryName();
            if (auto&& pWidget = GetWidget(widgetName)) {
                return pWidget.StaticCast<T>();
            }
            return nullptr;
        }

        template<typename T> SR_HTYPES_NS::SharedPtr<T> OpenWidget() {
            if (auto&& pWidget = GetWidget<T>()) {
                pWidget->Open();
                return pWidget;
            }
            return nullptr;
        }


        void SetRenderScene(const RenderScenePtr& renderScene);
        void SetRenderContext(ContextPtr pContext);

    public:
        SR_NODISCARD Widgets& GetWidgets() { return m_widgets; }
        SR_NODISCARD RenderScenePtr GetRenderScene() const;
        SR_NODISCARD ContextPtr GetContext() const;

        template<typename T> T* GetWidget() const {
            for (auto&& widget : m_widgets) {
                if (auto&& pWidget = dynamic_cast<T*>(widget.second)) {
                    return dynamic_cast<T*>(widget.second);
                }
            }

            SRHalt("WidgetManager::GetWidget() : unable to find widget!");
            return nullptr;
        }

        void OnMouseMove(const SR_UTILS_NS::MouseInputData* data) override;
        void OnMousePress(const SR_UTILS_NS::MouseInputData* data) override;
        void OnMouseDown(const SR_UTILS_NS::MouseInputData* data) override;
        void OnMouseUp(const SR_UTILS_NS::MouseInputData* data) override;

        void OnKeyDown(const SR_UTILS_NS::KeyboardInputData* data) override;
        void OnKeyUp(const SR_UTILS_NS::KeyboardInputData* data) override;
        void OnKeyPress(const SR_UTILS_NS::KeyboardInputData* data) override;

        void SetScene(const ScenePtr& scene);

    protected:
        mutable std::recursive_mutex m_mutex;

    private:
        RenderScenePtr m_renderScene;
        ContextPtr m_renderContext = nullptr;
        Widgets m_widgets;
        bool m_ignoreNonFocused;

    };

    class GlobalWidgetManager : public WidgetManager, public SR_UTILS_NS::Singleton<GlobalWidgetManager> {
        SR_REGISTER_SINGLETON(GlobalWidgetManager)
    public:
        ~GlobalWidgetManager() override = default;
    };

    class ViewportsTableManager : public SR_UTILS_NS::Singleton<ViewportsTableManager> {
        SR_REGISTER_SINGLETON(ViewportsTableManager)
    public:
        SR_NODISCARD ViewportsTable& GetViewportsTable() { return m_viewports; }
        SR_NODISCARD Widget* GetWidgetByViewport(void* viewport) const;

        void RegisterWidget(Widget* widget, void* viewport);

    private:
        ViewportsTable m_viewports;
    };
}

#endif //SR_ENGINE_WIDGETMANAGER_H
