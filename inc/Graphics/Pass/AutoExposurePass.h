//
// Created by Monika on 26.02.2026.
//

#ifndef SR_ENGINE_GRAPHICS_AUTO_EXPOSURE_PASS_PASS_H
#define SR_ENGINE_GRAPHICS_AUTO_EXPOSURE_PASS_PASS_H

#include <Graphics/Pass/BasePass.h>

namespace SR_GRAPH_NS {
    class AutoExposurePass : public BasePass {
        SR_CLASS()
        using Super = BasePass;
    protected:
        bool Prepare() override { return true; }
        bool Render() override { return true; }
        void Update() override { return; }

        bool Init() override { return true; }
        void DeInit() override { }

        void OnResize(const SR_MATH_NS::UVector2& size) override { }

    };
}

#endif //SR_ENGINE_GRAPHICS_AUTO_EXPOSURE_PASS_PASS_H
