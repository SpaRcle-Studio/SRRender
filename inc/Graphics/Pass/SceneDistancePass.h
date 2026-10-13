//
// Created by Monika on 13.10.2026.
//

#ifndef SR_ENGINE_GRAPHICS_SCENE_DISTANCE_PASS_H
#define SR_ENGINE_GRAPHICS_SCENE_DISTANCE_PASS_H

#include <Graphics/Pass/PostProcessPass.h>

namespace SR_GRAPH_NS {
    /// Дистанция до ближайшей поверхности в пониженном разрешении. Считается один раз за кадр
    /// и используется проходами пониженного разрешения (облака, туман, SSR) вместо чтения G-buffer.
    class SceneDistancePass : public PostProcessPass {
        SR_CLASS()
        using Super = PostProcessPass;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<SceneDistancePass>;

    public:
        bool Init() override;

    };
}

#endif //SR_ENGINE_GRAPHICS_SCENE_DISTANCE_PASS_H
