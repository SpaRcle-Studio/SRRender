//
// Created by Monika on 12.11.2026.
//

#ifndef SR_ENGINE_GLSL_GENERATOR_INTERNAL_H
#define SR_ENGINE_GLSL_GENERATOR_INTERNAL_H

#include <Graphics/SRSL/CodeGen/Common.h>
#include <Graphics/SRSL/ShaderType.h>
#include <Graphics/SRSL/RefAnalyzer.h>

namespace SR_SRSL_NS {
    class SRSLUniformBlock;

    class GLSLGenerator : public CodeGenDetail::Context {
    public:
        GLSLGenerator() = default;
        ~GLSLGenerator() override = default;

    public:
        SR_NODISCARD SRSLCodeGenRes GenerateStages(SR_UTILS_NS::IAllocator* pAllocator, const SRSLShader* pShader) override;

    private:
        SR_NODISCARD SR_UTILS_NS::String GenerateStage(ShaderStage stage, const SR_UTILS_NS::String& preCode = SR_UTILS_NS::String());

        SR_NODISCARD SR_UTILS_NS::String GenerateVertexStage();
        SR_NODISCARD SR_UTILS_NS::String GenerateFragmentStage();
        SR_NODISCARD SR_UTILS_NS::String GenerateComputeStage();

        SR_NODISCARD SR_UTILS_NS::StringView ReplaceToken(SR_UTILS_NS::StringView token, bool isUniform = false) const;
        SR_NODISCARD SR_UTILS_NS::StringView GetVersion(ShaderStage stage) const;
        SR_NODISCARD SR_UTILS_NS::String GenerateInputLocations(ShaderStage stage) const;
        SR_NODISCARD SR_UTILS_NS::String GenerateOutputLocations(ShaderStage stage) const;
        SR_NODISCARD SR_UTILS_NS::String GenerateUniforms(ShaderStage stage) const;

        SR_NODISCARD SR_UTILS_NS::String GenerateStructure(const SRSLStructureStatement* pStructure, int32_t deep) const;

        SR_NODISCARD SR_UTILS_NS::String GenerateType(const SRSLExpr* pExpr, int32_t deep) const;

        void AppendVariable(SR_UTILS_NS::String& code, const SRSLVariable* pVariable, int32_t deep) const;
        void AppendFunction(SR_UTILS_NS::String& code, const SRSLFunction* pFunction, int32_t deep) const;

        void AppendFunction(SR_UTILS_NS::String& code, const SRSLFunction* pFunction, int32_t deep,
                            const SR_UTILS_NS::String& preCode, const SR_UTILS_NS::String& postCode) const;

        void AppendStructure(SR_UTILS_NS::String& code, const SRSLStructureStatement* pStructure, int32_t deep) const;
        void AppendType(SR_UTILS_NS::String& code, const SRSLExpr* pExpr, int32_t deep) const;
        void AppendName(SR_UTILS_NS::String& code, const SRSLExpr* pExpr, int32_t deep) const;
        void AppendLexicalTree(SR_UTILS_NS::String& code, const SRSLLexicalTree* pLexicalTree, int32_t deep) const;

        void AppendLexicalTree(SR_UTILS_NS::String& code, const SRSLLexicalTree* pLexicalTree, int32_t deep,
                               const SR_UTILS_NS::String& preCode, const SR_UTILS_NS::String& postCode) const;

        void AppendIfStatement(SR_UTILS_NS::String& code, const SRSLIfStatement* pIfStatement, int32_t deep) const;
        void AppendWhileStatement(SR_UTILS_NS::String& code, const SRSLWhileStatement* pWhileStatement,
                                  int32_t deep) const;
        void AppendForStatement(SR_UTILS_NS::String& code, const SRSLForStatement* pForStatement, int32_t deep) const;

        void AppendExpression(SR_UTILS_NS::String& code, const SRSLExpr* expression, int32_t deep = 0) const;

        SR_NODISCARD SR_UTILS_NS::String GenerateExpression(const SRSLExpr* pExpr, int32_t deep) const;
        SR_NODISCARD SR_UTILS_NS::String GenerateVariable(const SRSLVariable* pVariable, int32_t deep) const;

        SR_NODISCARD uint32_t GetLocationMultiplier(const SR_UTILS_NS::StringView& type) const;

        SR_NODISCARD bool IsFlatType(const SR_UTILS_NS::StringView type) const;

        void GenerateSSBOBlock(SR_UTILS_NS::String& code, SR_UTILS_NS::StringAtom name,
                               const SRSLUniformBlock& uniformBlock, const SRSLUseStack::Ptr& pFunction) const;

    private:
        const SRSLShader* m_shader = nullptr;
    };
} // namespace SR_SRSL_NS

#endif // SR_ENGINE_GLSL_GENERATOR_INTERNAL_H
