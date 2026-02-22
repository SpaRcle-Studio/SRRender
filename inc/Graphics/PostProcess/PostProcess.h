//
// Created by Monika on 13.02.2026.
//

#ifndef SR_ENGINE_GRAPHICS_POST_PROCESS_COMPONENT_H
#define SR_ENGINE_GRAPHICS_POST_PROCESS_COMPONENT_H

#include <Graphics/stdInclude.h>

#include <Utils/ECS/Component.h>
#include <Utils/ECS/EntityRef.h>
#include <Utils/Resources/Asset.h>
#include <Utils/Resources/ResourceRef.h>

namespace SR_GRAPH_NS {
    class SkyboxPass;
    class RenderScene;
}

namespace SR_GTYPES_NS {
    class Camera;

    /// @extension(postprocess)
    class PostProcessSettings : public SR_UTILS_NS::Asset {
        SR_CLASS()
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<PostProcessSettings>;
    public:
        /// @property
        /// @customArgs(pick: enabled, filter name: Shader, relative: resources)
        /// @customArg(filter value: srsl)
        SR_UTILS_NS::Path shaderPath;

    };

    /// @displayName(PostProcess) @category(Render)
    class PostProcessComponent : public SR_UTILS_NS::Component {
        SR_CLASS()
        using Super = Component;
    public:
        void Update(float_t dt) override { }
        void OnDisable() override { }
        void OnDetached() override { }

        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }

    private:
        mutable SR_HTYPES_NS::SharedPtr<RenderScene> m_renderScene;

    private:
        /// @property
        SR_UTILS_NS::EntityRef<Camera> m_camera;
        /// @property
        /// @customArgs(pick: enabled, filter name: Post process, relative: resources)
        /// @customArg(filter value: postprocess)
        SR_UTILS_NS::ResourceRef<PostProcessSettings> m_settings;

    };
}

#endif //SR_ENGINE_GRAPHICS_POST_PROCESS_COMPONENT_H
