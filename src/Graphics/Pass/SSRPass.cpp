//
// Created by Monika on 13.10.2026.
//

#include <Graphics/Pass/SSRPass.h>
#include <Graphics/Types/Shader.h>

#include <Codegen/SSRPass.generated.hpp>

namespace SR_GRAPH_NS {
    bool SSRPass::Init() {
        SetShader("Engine/Shaders/SSR/ssr.srsl");

        if (auto&& pMaterialData = GetMaterial()->GetMaterialData()) {
            switch (stage) {
                case SSRPassStage::Trace: pMaterialData->AddShaderDefine("SR_SSR_TRACE"); break;
                case SSRPassStage::Resolve: pMaterialData->AddShaderDefine("SR_SSR_RESOLVE"); break;
                default:
                    break;
            }
        }

        return Super::Init();
    }

    void SSRPass::UseSharedUniforms(SR_GTYPES_NS::Shader& shader) {
        static const SR_UTILS_NS::StringAtom ssrTraceParams = "ssrTraceParams";
        static const SR_UTILS_NS::StringAtom ssrFadeParams = "ssrFadeParams";

        shader.SetVec4(ssrTraceParams, SR_MATH_NS::FVector4(
            static_cast<float_t>(SR_MATH_NS::Clamp<uint32_t>(maxSteps, 1, 256)),
            static_cast<float_t>(SR_MATH_NS::Clamp<uint32_t>(refineSteps, 0, 16)),
            SR_MATH_NS::Max(maxDistance, 0.1f),
            SR_MATH_NS::Max(thickness, 0.001f)
        ));
        shader.SetVec4(ssrFadeParams, SR_MATH_NS::FVector4(
            SR_MATH_NS::Clamp(maxRoughness, 0.01f, 1.f),
            SR_MATH_NS::Clamp(edgeFade, 0.001f, 0.5f),
            SR_MATH_NS::Max(blurRadius, 1.f),
            0.f
        ));
    }
}
