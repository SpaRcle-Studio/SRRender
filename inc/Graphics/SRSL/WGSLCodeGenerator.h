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

        std::string_view GenerateFunction(
            SRSLFunction* pFunction,
            const int32_t deep,
            const std::string_view& preArgs = std::string_view(),
            const std::string_view& preCode = std::string_view(),
            const std::string_view& postCode = std::string_view(),
            const std::string_view& returnType = std::string_view()
        );

        std::string_view GenerateStage(const SRSLShader* pShader, SRSLResult& result, ShaderStage stage, const std::string& preCode = std::string());
        std::optional<std::string_view> GenerateVertexStage(const SRSLShader* pShader, SRSLResult& result);
        std::optional<std::string_view> GenerateFragmentStage(const SRSLShader* pShader, SRSLResult& result);
        std::optional<std::string_view> GenerateComputeStage(const SRSLShader* pShader, SRSLResult& result);

    };
}

#endif //SR_ENGINE_GRAPHICS_WGSL_CODE_GENERATOR_H
