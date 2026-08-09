//
// Created by Monika on 22.01.2023.
//

#ifndef SR_ENGINE_SRSL_LEXICAL_ANALYZER_H
#define SR_ENGINE_SRSL_LEXICAL_ANALYZER_H

#include <Graphics/SRSL/MathExpression.h>

#include <Utils/Common/Singleton.h>

namespace SR_SRSL_NS {
    class SRSLLexicalAnalyzer : public SR_UTILS_NS::Singleton<SRSLLexicalAnalyzer> {
        SR_REGISTER_SINGLETON(SRSLLexicalAnalyzer)
    private:
        enum class LXAState {
            Decorators, Decorator, DecoratorArgs,
            Expression, Variable, Function, FunctionArgs, FunctionBody, IfStatement, IfStatementBody,
            ForStatement, ForStatementVariable, ForStatementCondition, ForStatementExpression, ForStatementBody,
            WhileStatement, WhileStatementCondition, WhileStatementBody,
            StructureStatement, StructureStatementBody,
        };
    public:
        SR_NODISCARD std::pair<SRSLAnalyzedTree*, SR_UTILS_NS::LexerDetails::LexerResult> Analyze(SR_UTILS_NS::IAllocator* pAllocator, std::span<SR_UTILS_NS::LexerDetails::Lexem> lexems);

    private:
        void Clear();

        void ProcessMain();
        bool ProcessInBuiltName();
        void ProcessBracket();
        void ProcessDecorators();
        void ProcessExpression(bool isFunctionName = false, bool isSimpleExpr = false);

        SR_NODISCARD SRSLLexicalUnit* TryProcessIdentifier();

        SR_NODISCARD bool InBounds() const noexcept;
        SR_NODISCARD bool IsHasErrors() const noexcept;
        SR_NODISCARD const SR_UTILS_NS::LexerDetails::Lexem* GetLexem(int64_t offset) const;
        SR_NODISCARD const SR_UTILS_NS::LexerDetails::Lexem* GetCurrentLexem() const;

    private:
        SR_UTILS_NS::Vector<SRSLLexicalTree*> m_lexicalTree;
        SR_UTILS_NS::IAllocator* m_pAllocator = nullptr;
        SR_UTILS_NS::Vector<SR_UTILS_NS::LexerDetails::Lexem> m_exprLexems;

        SRSLDecorators* m_decorators = nullptr;
        SRSLExpr* m_expr = nullptr;

        SR_UTILS_NS::LexerDetails::LexerResult m_result;
        int64_t m_currentLexem = 0;

        SR_UTILS_NS::Vector<LXAState> m_states;
        std::span<SR_UTILS_NS::LexerDetails::Lexem> m_lexems;

    };
}

#endif //SR_ENGINE_SRSL_LEXICAL_ANALYZER_H