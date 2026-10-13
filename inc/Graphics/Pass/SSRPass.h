//
// Created by Monika on 13.10.2026.
//

#ifndef SR_ENGINE_GRAPHICS_SSR_PASS_H
#define SR_ENGINE_GRAPHICS_SSR_PASS_H

#include <Graphics/Pass/PostProcessPass.h>

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(SSRPassStage, uint8_t,
        Trace,
        Resolve
    );

    /// Screen space reflections: трассировка лучей по G-buffer (Trace) и размытие по шероховатости (Resolve).
    class SSRPass : public PostProcessPass {
        SR_CLASS()
        using Super = PostProcessPass;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<SSRPass>;

    public:
        bool Init() override;

    protected:
        void UseSharedUniforms(SR_GTYPES_NS::Shader& shader) override;

    public:
        /// @property
        SSRPassStage stage = SSRPassStage::Trace;
        /// @property
        uint32_t maxSteps = 48;
        /// @property
        uint32_t refineSteps = 5;
        /// @property
        float_t maxDistance = 100.f;
        /// @property
        float_t thickness = 0.3f;
        /// @property
        float_t maxRoughness = 0.6f;
        /// @property
        float_t edgeFade = 0.1f;
        /// @property
        float_t blurRadius = 6.f;

    };
}

#endif //SR_ENGINE_GRAPHICS_SSR_PASS_H
