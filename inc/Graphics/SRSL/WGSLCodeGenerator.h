//
// Created by Monika on 11.04.2026.
//

#ifndef SR_ENGINE_GRAPHICS_WGSL_CODE_GENERATOR_H
#define SR_ENGINE_GRAPHICS_WGSL_CODE_GENERATOR_H

#include <Graphics/SRSL/ICodeGenerator.h>
#include <Graphics/SRSL/ShaderType.h>
#include <Graphics/SRSL/RefAnalyzer.h>

namespace SR_SRSL_NS {
    class SRSLUniformBlock;

    class WGSLCodeGenerator : public ISRSLCodeGenerator, public SR_UTILS_NS::Singleton<WGSLCodeGenerator> {
    SR_REGISTER_SINGLETON(WGSLCodeGenerator)
    private:
        WGSLCodeGenerator() = default;
        ~WGSLCodeGenerator() override = default;

    public:
        SR_NODISCARD SRSLCodeGenRes GenerateStages(const SRSLShader* pShader) override;

    };
}

#endif //SR_ENGINE_GRAPHICS_WGSL_CODE_GENERATOR_H
