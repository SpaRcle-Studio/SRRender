#include <Graphics/SRSL/GLSLCodeGenerator.h>
#include <Graphics/SRSL/CodeGen/GLSLGenerator.h>
#include <Graphics/SRSL/Shader.h>
#include <Graphics/SRSL/ShaderVariables.h>

namespace SR_SRSL_NS {
    ISRSLCodeGenerator::SRSLCodeGenRes GLSLCodeGenerator::GenerateStages(SR_UTILS_NS::IAllocator* allocator, const SRSLShader* shader) {
        SR_TRACY_ZONE;
        SR_GLOBAL_LOCK
        GLSLGenerator generator;
        return generator.GenerateStages(allocator, shader);
    }

    ISRSLCodeGenerator::SRSLCodeGenRes GLSLGenerator::GenerateStages(SR_UTILS_NS::IAllocator*, const SRSLShader* pShader) {
        SR_TRACY_ZONE;

        Clear();

        m_shader = pShader;

        ISRSLCodeGenerator::SRSLCodeGenRes codeGenRes;

        auto&& [result, stages] = codeGenRes;

        if (!pShader || !pShader->GetAnalyzedTree() || !pShader->GetAnalyzedTree()->pLexicalTree) {
            result = SR_UTILS_NS::LexerDetails::LexerResult(SR_UTILS_NS::LexerDetails::LexerReturnCode::InvalidLexicalTree);
            return codeGenRes;
        }

        if (auto code = GenerateVertexStage(); !code.empty()) {
            stages[ShaderStage::Vertex] = std::move(code);
        }

        if (auto code = GenerateFragmentStage(); !code.empty()) {
            stages[ShaderStage::Fragment] = std::move(code);
        }

        if (auto code = GenerateComputeStage(); !code.empty()) {
            stages[ShaderStage::Compute] = std::move(code);
        }

        result = SR_UTILS_NS::Exchange(m_result, { });

        return codeGenRes;
    }

}
