//
// Created by Monika on 13.02.2026.
//

#ifndef SR_ENGINE_GRAPHICS_SKYBOX_COMPONENT_H
#define SR_ENGINE_GRAPHICS_SKYBOX_COMPONENT_H

#include <Graphics/stdInclude.h>

#include <Utils/ECS/Component.h>
#include <Utils/ECS/EntityRef.h>
#include <Utils/FileSystem/Path.h>

namespace SR_GRAPH_NS {
    class SkyboxPass;
    class RenderScene;
}

namespace SR_GTYPES_NS {
    class Camera;

    /// @displayName(Skybox) @category(Render)
    class SkyboxComponent : public SR_UTILS_NS::Component {
        SR_CLASS()
        using Super = Component;
    public:
        void Update(float_t dt) override;
        void OnDisable() override;
        void OnDetached() override;

        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }
        void SetParams(const SR_UTILS_NS::Path& skyboxPath, const SR_UTILS_NS::Path& shaderPath, bool isQuad);

    private:
        SR_NODISCARD SkyboxPass* FindSkyboxPass() const;
        SR_NODISCARD RenderScene* GetRenderScene() const;

    private:
        mutable SR_HTYPES_NS::SharedPtr<RenderScene> m_renderScene;

    private:
        /// @property
        SR_UTILS_NS::EntityRef<Camera> m_camera;
        /// @property
        /// @customArgs(pick: enabled, filter name: Shader, relative: resources)
        /// @customArg(filter value: srsl)
        SR_UTILS_NS::Path m_shaderPath;
        /// @property
        SR_UTILS_NS::Path m_skyboxPath;
        /// @property
        bool m_isQuad = false;

    };
}

#endif //SR_ENGINE_GRAPHICS_SKYBOX_COMPONENT_H
