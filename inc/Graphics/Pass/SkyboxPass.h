//
// Created by Monika on 14.07.2022.
//

#ifndef SR_ENGINE_GRAPHICS_SKYBOX_PASS_H
#define SR_ENGINE_GRAPHICS_SKYBOX_PASS_H

#include <Graphics/Pass/BasePass.h>

namespace SR_GTYPES_NS {
    class Skybox;
}

namespace SR_GRAPH_NS {
    class SkyboxPass : public BasePass {
        using Super = BasePass;
        SR_CLASS()
    public:
        ~SkyboxPass() override;

    public:
        bool Render() override;
        void Update() override;

        void SetSkybox(const SR_UTILS_NS::Path& path);
        void SetShader(const SR_UTILS_NS::Path& path);

    private:
        SR_HTYPES_NS::SharedPtr<SR_GTYPES_NS::Skybox> m_skybox;
        bool m_isRendered = false;

        /// @property @setter(SetSkybox)
        SR_UTILS_NS::Path m_skyboxPath;
        /// @property @setter(SetShader)
        /// @customArgs(pick: enabled, filter name: Shader, relative: resources)
        /// @customArg(filter value: srsl)
        SR_UTILS_NS::Path m_shaderPath;

    };
}

#endif //SR_ENGINE_GRAPHICS_SKYBOX_PASS_H
