//
// Created by Nariman on 07.05.2026.
//

#ifndef SR_ENGINE_GRAPHICS_BLUR_PASS_H
#define SR_ENGINE_GRAPHICS_BLUR_PASS_H

#include <Graphics/Pass/PostProcessPass.h>

namespace SR_GRAPH_NS {
    class BlurPass : public PostProcessPass {
        SR_CLASS()
        using Super = PostProcessPass;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<BlurPass>;

    public:
        bool Init() override;

    };
}

#endif //SR_ENGINE_GRAPHICS_BLUR_PASS_H
