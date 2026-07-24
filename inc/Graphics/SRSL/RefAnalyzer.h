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
    struct SRSLUseStackHolder;
    struct SRSLUseStack {
        using Ptr = std::shared_ptr<SRSLUseStack>;

        SR_NODISCARD SR_UTILS_NS::Set<SR_GRAPH_NS::ShaderStage> IsVariableUsedInEntryPointsExt(const std::string_view& name) const;
        SR_NODISCARD bool IsVariableUsedInEntryPoints(const std::string_view& name) const;
        SR_NODISCARD bool IsVariableUsedInEntryPoint(SR_GRAPH_NS::ShaderStage stage, const std::string_view& name) const;
        SR_NODISCARD bool IsVariableUsed(const std::string_view& name, uint8_t depth = 0) const;
        SR_NODISCARD bool IsFunctionUsed(const std::string_view& name, uint8_t depth = 0) const;
        SR_NODISCARD bool IsStructUsed(const std::string_view &name) const;

        SR_NODISCARD SRSLUseStack::Ptr FindFunction(const std::string_view& name) const;

        SR_NODISCARD std::string ToString(int32_t deep) const;

        void Concat(const SRSLUseStack::Ptr& pOther);
        void SetRoot(SRSLUseStack* pRootStack);

        SR_UTILS_NS::Map<SR_UTILS_NS::String, SRSLUseStack::Ptr> functions;

        SR_HTYPES_NS::SortedVector<SR_UTILS_NS::StringView> variables;
        SR_HTYPES_NS::SortedVector<SR_UTILS_NS::StringView> forceUsedVariables;
        SR_HTYPES_NS::SortedVector<SR_UTILS_NS::StringView> forceUsedFunctions;

        SRSLUseStack* pRoot = nullptr;
    };

    struct SRSLUseStackHolder {
        using Ptr = SR_HTYPES_NS::RawPointerHolder<SRSLUseStackHolder>;

        
    };

    class SRSLRefAnalyzer : public SR_UTILS_NS::Singleton<SRSLRefAnalyzer> {
        SR_REGISTER_SINGLETON(SRSLRefAnalyzer)
        using Stack = SR_UTILS_NS::Vector<SR_UTILS_NS::StringView>;
    public:
        SR_NODISCARD SRSLUseStack::Ptr Analyze(const SRSLAnalyzedTree* pAnalyzedTree, const SR_SRSL_NS::ShaderParams& params);

    private:
        SR_NODISCARD SRSLFunction* FindFunction(const std::string& name) const;
        SR_NODISCARD SRSLFunction* FindFunction(SRSLLexicalTree* pTree, const std::string& name) const;
        SR_NODISCARD SRSLUseStack::Ptr AnalyzeTree(Stack& stack, SRSLLexicalTree* pTree);

        void PreprocessUseStack(SRSLUseStack::Ptr& pUseStack, const SR_SRSL_NS::ShaderParams& params);

        void AnalyzeVariable(SRSLUseStack::Ptr& pUseStack, Stack& stack, SRSLVariable* pVariable);
        void AnalyzeExpression(SRSLUseStack::Ptr& pUseStack, Stack& stack, SRSLExpr* pExpr);
        void AnalyzeArrayExpression(SRSLUseStack::Ptr& pUseStack, Stack& stack, SRSLExpr* pExpr);
        void AnalyzeIfStatement(SRSLUseStack::Ptr& pUseStack, Stack& stack, SRSLIfStatement* pIfStatement);
        void AnalyzeForStatement(SRSLUseStack::Ptr& pUseStack, Stack& stack, SRSLForStatement* pForStatement);
        void AnalyzeWhileStatement(SRSLUseStack::Ptr& pUseStack, Stack& stack, SRSLWhileStatement* pWhileStatement);
        void AnalyzeFunction(SRSLUseStack::Ptr& pUseStack, Stack& stack, SRSLFunction* pFunction);

    private:
        const SRSLAnalyzedTree* m_analyzedTree = nullptr;

    };
}

#endif //SR_ENGINE_REFANALYZER_H
