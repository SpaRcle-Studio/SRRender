//
// Created by Monika on 30.01.2023.
//

#ifndef SRENGINE_GLSLCODEGENERATOR_H
#define SRENGINE_GLSLCODEGENERATOR_H

#include <Graphics/SRSL/ICodeGenerator.h>
#include <Graphics/SRSL/ShaderType.h>

namespace SR_SRSL_NS {
    class GLSLCodeGenerator : public ISRSLCodeGenerator, public SR_UTILS_NS::Singleton<GLSLCodeGenerator> {
        friend class SR_UTILS_NS::Singleton<GLSLCodeGenerator>;
    private:
        GLSLCodeGenerator() = default;
        ~GLSLCodeGenerator() override = default;

    public:
        SR_NODISCARD SRSLCodeGenRes GenerateStages(const SRSLShader* pShader) override;

    private:
        SR_NODISCARD std::optional<std::string> GenerateVertexStage();
        SR_NODISCARD std::optional<std::string> GenerateFragmentStage();

        SR_NODISCARD std::string GetVersion(ShaderStage stage) const;
        SR_NODISCARD std::string GenerateVertexLocations() const;
        SR_NODISCARD std::string GenerateUniformBlocks(ShaderStage stage) const;
        SR_NODISCARD std::string GenerateSamplers(ShaderStage stage) const;

        SR_NODISCARD std::string GenerateLexicalTree(SRSLLexicalTree* pLexicalTree, int32_t deep) const;
        SR_NODISCARD std::string GenerateVariable(SRSLVariable* pVariable, int32_t deep) const;
        SR_NODISCARD std::string GenerateFunction(SRSLFunction* pFunction, int32_t deep) const;

        SR_NODISCARD std::string GenerateType(SRSLExpr* pExpr, int32_t deep) const;
        SR_NODISCARD std::string GenerateName(SRSLExpr* pExpr, int32_t deep) const;

        SR_NODISCARD std::string GenerateExpression(SRSLExpr* pExpr, int32_t deep) const;

        SR_NODISCARD std::string GenerateTab(int32_t deep) const;

    private:
        const SRSLShader* m_shader = nullptr;

    };
}

#endif //SRENGINE_GLSLCODEGENERATOR_H