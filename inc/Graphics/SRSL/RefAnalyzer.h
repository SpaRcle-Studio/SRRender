//
// Created by Monika on 03.02.2023.
//

#ifndef SR_ENGINE_REFANALYZER_H
#define SR_ENGINE_REFANALYZER_H

#include <Graphics/SRSL/LexicalTree.h>

namespace SR_GRAPH_NS {
    enum class ShaderStage : uint8_t;
}

namespace SR_SRSL_NS {
    struct SRSLUseStack {
        using Ptr = std::shared_ptr<SRSLUseStack>;

        SR_NODISCARD std::set<SR_GRAPH_NS::ShaderStage> IsVariableUsedInEntryPointsExt(const std::string& name) const;
        SR_NODISCARD bool IsVariableUsedInEntryPoints(const std::string& name) const;
        SR_NODISCARD bool IsVariableUsedInEntryPoint(SR_GRAPH_NS::ShaderStage stage, const std::string& name) const;
        SR_NODISCARD bool IsVariableUsed(const std::string& name) const;
        SR_NODISCARD bool IsFunctionUsed(const std::string& name) const;
        SR_NODISCARD bool IsStructUsed(const std::string &name) const;

        SR_NODISCARD SRSLUseStack::Ptr FindFunction(const std::string& name) const;

        SR_NODISCARD std::string ToString(int32_t deep) const;

        void Concat(const SRSLUseStack::Ptr& pOther);
        void SetRoot(SRSLUseStack* pRootStack);

        std::map<std::string, SRSLUseStack::Ptr> functions;
        std::set<std::string> variables;

        std::set<std::string> forceUsedVariables;
        std::set<std::string> forceUsedFunctions;

        SRSLUseStack* pRoot = nullptr;
    };

    class SRSLRefAnalyzer : public SR_UTILS_NS::Singleton<SRSLRefAnalyzer> {
        SR_REGISTER_SINGLETON(SRSLRefAnalyzer)
    public:
        SR_NODISCARD SRSLUseStack::Ptr Analyze(const SRSLAnalyzedTree::Ptr& pAnalyzedTree, const SR_SRSL_NS::ShaderMacrosParams& macros);

    private:
        SR_NODISCARD SRSLFunction* FindFunction(const std::string& name) const;
        SR_NODISCARD SRSLFunction* FindFunction(SRSLLexicalTree* pTree, const std::string& name) const;
        SR_NODISCARD SRSLUseStack::Ptr AnalyzeTree(std::list<std::string>& stack, SRSLLexicalTree* pTree);

        void PreprocessUseStack(SRSLUseStack::Ptr& pUseStack, const SR_SRSL_NS::ShaderMacrosParams& macros);

        void AnalyzeVariable(SRSLUseStack::Ptr& pUseStack, std::list<std::string>& stack, SRSLVariable* pVariable);
        void AnalyzeExpression(SRSLUseStack::Ptr& pUseStack, std::list<std::string>& stack, SRSLExpr* pExpr);
        void AnalyzeArrayExpression(SRSLUseStack::Ptr& pUseStack, std::list<std::string>& stack, SRSLExpr* pExpr);
        void AnalyzeIfStatement(SRSLUseStack::Ptr& pUseStack, std::list<std::string>& stack, SRSLIfStatement* pIfStatement);
        void AnalyzeForStatement(SRSLUseStack::Ptr& pUseStack, std::list<std::string>& stack, SRSLForStatement* pForStatement);
        void AnalyzeWhileStatement(SRSLUseStack::Ptr& pUseStack, std::list<std::string>& stack, SRSLWhileStatement* pWhileStatement);
        void AnalyzeFunction(SRSLUseStack::Ptr& pUseStack, std::list<std::string>& stack, SRSLFunction* pFunction);

    private:
        SRSLAnalyzedTree::Ptr m_analyzedTree;

    };
}

#endif //SR_ENGINE_REFANALYZER_H
