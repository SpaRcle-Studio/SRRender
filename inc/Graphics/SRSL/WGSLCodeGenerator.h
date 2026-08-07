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
        SR_NODISCARD SRSLCodeGenRes GenerateStages(SR_UTILS_NS::IAllocator* pAllocator, const SRSLShader* pShader) override;

        /// Entry-point function wrapper (adds @vertex / @fragment / @compute decorator in the caller).
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

    private:
        SR_NODISCARD SR_UTILS_NS::String GenerateConstants(const SRSLShader* pShader) const;

        /// Non-entry-point function (regular function body without stage decorator).
        SR_NODISCARD std::string GenerateFunctionBody(const SRSLFunction* pFunction, int32_t deep) const;

        SR_NODISCARD std::string GenerateLexicalTree(const SRSLLexicalTree* pLexicalTree, int32_t deep) const;
        SR_NODISCARD std::string GenerateLexicalTree(const SRSLLexicalTree* pLexicalTree, int32_t deep, const std::string& preCode, const std::string& postCode) const;

        SR_NODISCARD std::string GenerateExpression(const SRSLExpr* pExpr, int32_t deep) const;
        SR_NODISCARD std::string GenerateVariable(const SRSLVariable* pVariable, int32_t deep) const;
        SR_NODISCARD std::string GenerateStructure(const SRSLStructureStatement* pStructure, int32_t deep) const;

        SR_NODISCARD std::string GenerateIfStatement(const SRSLIfStatement* pIfStatement, int32_t deep) const;
        SR_NODISCARD std::string GenerateForStatement(const SRSLForStatement* pForStatement, int32_t deep) const;
        SR_NODISCARD std::string GenerateWhileStatement(const SRSLWhileStatement* pWhileStatement, int32_t deep) const;

        SR_NODISCARD std::string GenerateUniforms(const SRSLShader* pShader) const;

        /// Generates function-local `let` aliases for push constant fields used in the given stage.
        SR_NODISCARD std::string GeneratePushConstantAliases(const SRSLShader* pShader, ShaderStage stage) const;

        /// Generates function-local `let` aliases for all UBO block fields used in the given stage.
        /// In GLSL UBO fields are globally visible; in WGSL they require `block.field` access.
        SR_NODISCARD std::string GenerateUniformBlockAliases(const SRSLShader* pShader, ShaderStage stage) const;

    private:
        mutable SR_UTILS_NS::String m_tmpBuffer;
        /// Set for the duration of a GenerateStages() call so that GenerateExpression()
        /// can look up sampler names when rewriting texture() calls.
        const SRSLShader* m_pCurrentShader = nullptr;
    };
}

#endif //SR_ENGINE_GRAPHICS_WGSL_CODE_GENERATOR_H
