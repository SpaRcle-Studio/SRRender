//
// Created by Monika on 22.05.2023.
//

#ifndef SR_ENGINE_I_RENDER_COMPONENT_H
#define SR_ENGINE_I_RENDER_COMPONENT_H

#include <Graphics/macros.h>

#include <Utils/ECS/Component.h>

namespace SR_GRAPH_NS {
    class RenderScene;
}

namespace SR_GTYPES_NS {
    class Camera;

    /// @hidden @category(Render)
    class IRenderComponent : public SR_UTILS_NS::Component {
        SR_CLASS()
        using Super = SR_UTILS_NS::Component;
    public:
        using RenderScenePtr = RenderScene*;
        using CameraPtr = SR_HTYPES_NS::SharedPtr<Camera>;
        using DrawFn = void(*)();
        using UpdateFn = void(*)();
        using BindFn = void(*)();

    public:
        void OnEnable() override;
        void OnDisable() override;

        SR_NODISCARD CameraPtr GetCamera() const;
        SR_NODISCARD RenderScenePtr TryGetRenderScene() const;
        SR_NODISCARD RenderScenePtr GetRenderScene() const;

    protected:
        mutable RenderScenePtr m_renderScene = nullptr;

    };

    constexpr static size_t SIZE_OF_I_RENDER_COMPONENT_CLASS = sizeof(IRenderComponent);
}

#endif //SR_ENGINE_I_RENDER_COMPONENT_H
