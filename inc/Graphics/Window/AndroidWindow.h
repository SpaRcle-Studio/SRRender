//
// Created by Monika on 08.12.2022.
//

#ifndef SR_ENGINE_ANDROIDWINDOW_H
#define SR_ENGINE_ANDROIDWINDOW_H

#include <Graphics/Window/BasicWindowImpl.h>

#include <Utils/Platform/AndroidNativeAppGlue.h>

#include <android/log.h>
#include <android/native_activity.h>
#include <android/configuration.h>

namespace SR_GRAPH_NS {
    class AndroidWindow : public BasicWindowImpl {
        using Super = BasicWindowImpl;
    public:
        explicit AndroidWindow()
            : Super()
        { }
        ~AndroidWindow() override;

    public:
        bool Initialize(const std::string& name,
                        const SR_MATH_NS::IVector2& position,
                        const SR_MATH_NS::UVector2& size,
                        bool fullScreen, bool resizable) override;

        SR_NODISCARD WindowType GetType() const override { return WindowType::Android; }

        void PollEvents() override;
        SR_NODISCARD bool IsSurfaceAvailable() const override;
        void SetSurfaceChangeCallback(SR_HTYPES_NS::Function<bool(ANativeWindow*)> callback);
        SR_NODISCARD SR_MATH_NS::IVector2 ScreenToClient(const SR_MATH_NS::IVector2& position) const override;
        SR_NODISCARD SR_MATH_NS::IVector2 ClientToScreen(const SR_MATH_NS::IVector2& position) const override;

        SR_NODISCARD ANativeWindow* GetNativeWindow() const;

        void* GetHandle() const override { return GetNativeWindow(); }

    private:
        ANativeWindow* m_nativeWindow = nullptr;
        uint64_t m_windowRevision = 0;
        bool m_resumed = false;
        bool m_surfaceReady = true;
        bool m_surfacePending = false;
        SR_HTYPES_NS::Function<bool(ANativeWindow*)> m_surfaceChange;

    };
}

#endif //SR_ENGINE_ANDROIDWINDOW_H
