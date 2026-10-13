//
// Created by Monika on 13.10.2026.
//

#ifndef SR_ENGINE_GRAPHICS_BLOOM_PASS_H
#define SR_ENGINE_GRAPHICS_BLOOM_PASS_H

#include <Graphics/Pass/PostProcessPass.h>

namespace SR_GRAPH_NS {
    SR_ENUM_NS_CLASS_T(BloomPassStage, uint8_t,
        Prefilter,
        Downsample,
        Upsample
    );

    /// Один шаг цепочки bloom: prefilter (порог + Karis average), downsample или upsample с накоплением.
    class BloomPass : public PostProcessPass {
        SR_CLASS()
        using Super = PostProcessPass;
    public:
        using Ptr = SR_HTYPES_NS::SharedPtr<BloomPass>;

    public:
        bool Init() override;

    protected:
        void UseSharedUniforms(SR_GTYPES_NS::Shader& shader) override;

    public:
        /// @property
        BloomPassStage stage = BloomPassStage::Downsample;
        /// @property
        bool highQuality = true;
        /// @property
        float_t threshold = 1.f;
        /// @property
        float_t knee = 0.5f;
        /// @property
        float_t clampMax = 64.f;
        /// @property
        float_t scatter = 0.7f;

    };
}

#endif //SR_ENGINE_GRAPHICS_BLOOM_PASS_H
