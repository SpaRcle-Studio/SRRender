//
// Created by Monika on 13.02.2026.
//

#ifndef SR_ENGINE_GRAPHICS_SKYBOX_COMPONENT_H
#define SR_ENGINE_GRAPHICS_SKYBOX_COMPONENT_H

#include <Graphics/Types/IRenderComponent.h>

#include <Utils/ECS/Component.h>
#include <Utils/ECS/EntityRef.h>
#include <Utils/FileSystem/Path.h>

namespace SR_GRAPH_NS {
    class RenderScene;
}

namespace SR_GTYPES_NS {
    class Skybox;

    /// @displayName(Skybox) @category(Render)
    class SkyboxComponent : public SR_GTYPES_NS::IRenderComponent {
        SR_CLASS()
        using Super = SR_GTYPES_NS::IRenderComponent;
    public:
        ~SkyboxComponent() override;

        const SR_UTILS_NS::VertexLayoutDescription& GetShaderVertexLayoutDescription() const noexcept override;
        SR_NODISCARD bool ExecuteInEditMode() const override { return true; }

        void SetParams(const SR_UTILS_NS::Path& skyboxPath, const SR_UTILS_NS::Path& shaderPath, bool isQuad);
        void Draw() override;

        int32_t GetVirtualUBO() const override { return m_virtualUBO; }

        void FreeVideoMemory() override;

    private:
        mutable SR_HTYPES_NS::SharedPtr<RenderScene> m_renderScene;

    private:
        SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Skybox> m_skybox;
        bool m_isRendered = false;
        bool m_isSkyboxDirty = true;

        /// @property
        SR_UTILS_NS::Path m_skyboxPath;
        /// @property
        bool m_isQuad = false;

        int32_t m_virtualUBO = SR_ID_INVALID;
        int32_t m_virtualDescriptor = SR_ID_INVALID;

    };
}

#endif //SR_ENGINE_GRAPHICS_SKYBOX_COMPONENT_H
