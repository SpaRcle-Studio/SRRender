//
// Created by Monika on 06.10.2026.
//

#ifndef SR_ENGINE_GRAPHICS_VOLUMETRIC_FOG_PASS_H
#define SR_ENGINE_GRAPHICS_VOLUMETRIC_FOG_PASS_H

#include <Graphics/Pass/PostProcessPass.h>

namespace SR_GRAPH_NS {
    /// Рендерит туман в буфер пониженного разрешения, если на сцене есть VolumetricFog.
    class VolumetricFogPass : public PostProcessPass {
        SR_CLASS()
        using Super = PostProcessPass;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<VolumetricFogPass>;

    public:
        bool Init() override;
        bool Render() override;

        void SetShadowPassName(SR_UTILS_NS::StringAtom name) { m_shadowPassName = name; }

    protected:
        void UseSharedUniforms(SR_GTYPES_NS::Shader& shader) override;

    private:
        /// @property
        SR_UTILS_NS::StringAtom m_shadowPassName;

    };
}

#endif //SR_ENGINE_GRAPHICS_VOLUMETRIC_FOG_PASS_H
