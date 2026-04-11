//
// Created by Monika on 08.04.2026.
//

#include <Graphics/Pipeline/ShaderUtils.h>

namespace SR_GRAPH_NS {
    bool SRShaderCreateInfo::Validate() const noexcept {
        if (stages.empty()) {
            SRHalt("SRShaderCreateInfo::Validate() : stages is empty!");
            return false;
        }

        if (shaderType == SR_SRSL_NS::ShaderType::Compute) {
            return polygonMode          == PolygonMode::Unknown
                   && cullMode          == CullMode::Unknown
                   && depthCompare      == DepthCompare::Unknown
                   && primitiveTopology == PrimitiveTopology::Unknown;
        }

        return polygonMode          != PolygonMode::Unknown
               && cullMode          != CullMode::Unknown
               && depthCompare      != DepthCompare::Unknown
               && primitiveTopology != PrimitiveTopology::Unknown;
    }

    SRShaderCreateInfo::SRShaderCreateInfo(SRShaderCreateInfo&& ref) noexcept
        : stages(std::move(ref.stages))
        , shaderType(ref.shaderType)
        , polygonMode(ref.polygonMode)
        , cullMode(ref.cullMode)
        , depthCompare(ref.depthCompare)
        , primitiveTopology(ref.primitiveTopology)
        , uniforms(std::move(ref.uniforms))
        , blendEnabled(ref.blendEnabled)
        , depthWrite(ref.depthWrite)
        , depthTest(ref.depthTest)
        , alphaCoverage(ref.alphaCoverage)
    {
        memmove(&vertexLayoutDescription, &ref.vertexLayoutDescription, sizeof(vertexLayoutDescription));
    }

    SRShaderCreateInfo& SRShaderCreateInfo::operator=(SRShaderCreateInfo&& ref) noexcept {
        if (this != &ref) {
            stages = std::move(ref.stages);
            shaderType = ref.shaderType;
            memmove(&vertexLayoutDescription, &ref.vertexLayoutDescription, sizeof(vertexLayoutDescription));
            polygonMode = ref.polygonMode;
            cullMode = ref.cullMode;
            depthCompare = ref.depthCompare;
            primitiveTopology = ref.primitiveTopology;
            uniforms = std::move(ref.uniforms);
            blendEnabled = ref.blendEnabled;
            depthWrite = ref.depthWrite;
            depthTest = ref.depthTest;
            alphaCoverage = ref.alphaCoverage;
        }
        return *this;
    }
}