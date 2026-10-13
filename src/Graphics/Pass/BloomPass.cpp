//
// Created by Monika on 13.10.2026.
//

#include <Graphics/Pass/BloomPass.h>
#include <Graphics/Types/Shader.h>

#include <Codegen/BloomPass.generated.hpp>

namespace SR_GRAPH_NS {
    bool BloomPass::Init() {
        SetShader("Engine/Shaders/Bloom/bloom.srsl");

        if (auto&& pMaterialData = GetMaterial()->GetMaterialData()) {
            switch (stage) {
                case BloomPassStage::Prefilter: pMaterialData->AddShaderDefine("SR_BLOOM_PREFILTER"); break;
                case BloomPassStage::Downsample: pMaterialData->AddShaderDefine("SR_BLOOM_DOWNSAMPLE"); break;
                case BloomPassStage::Upsample: pMaterialData->AddShaderDefine("SR_BLOOM_UPSAMPLE"); break;
                default:
                    break;
            }

            if (highQuality) {
                pMaterialData->AddShaderDefine("SR_BLOOM_HQ");
            }

            if (!m_useSSBOFromPasses.empty()) {
                pMaterialData->AddShaderDefine("SR_BLOOM_EXPOSURE");
            }
        }

        return Super::Init();
    }

    void BloomPass::UseSharedUniforms(SR_GTYPES_NS::Shader& shader) {
        static const SR_UTILS_NS::StringAtom bloomParams = "bloomParams";
        static const SR_UTILS_NS::StringAtom bloomCombineParams = "bloomCombineParams";

        const float_t safeKnee = SR_MATH_NS::Max(knee, 0.0001f);
        shader.SetVec4(bloomParams, SR_MATH_NS::FVector4(
            SR_MATH_NS::Max(threshold, 0.f),
            safeKnee,
            SR_MATH_NS::Max(clampMax, 0.001f),
            0.f
        ));
        shader.SetVec4(bloomCombineParams, SR_MATH_NS::FVector4(SR_MATH_NS::Clamp(scatter, 0.f, 1.f), 0.f, 0.f, 0.f));
    }
}
