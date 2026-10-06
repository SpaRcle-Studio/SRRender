//
// Created by Monika on 06.10.2026.
//

#ifndef SR_ENGINE_GRAPHICS_VOLUMETRIC_CLOUDS_PASS_H
#define SR_ENGINE_GRAPHICS_VOLUMETRIC_CLOUDS_PASS_H

#include <Graphics/Pass/PostProcessPass.h>

namespace SR_GRAPH_NS {
    /// Рендерит облака в буфер пониженного разрешения, если на сцене есть VolumetricClouds.
    class VolumetricCloudsPass : public PostProcessPass {
        SR_CLASS()
        using Super = PostProcessPass;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<VolumetricCloudsPass>;

    public:
        bool Init() override;
        bool Render() override;

    protected:
        void UseSharedUniforms(SR_GTYPES_NS::Shader& shader) override;

    };
}

#endif //SR_ENGINE_GRAPHICS_VOLUMETRIC_CLOUDS_PASS_H
