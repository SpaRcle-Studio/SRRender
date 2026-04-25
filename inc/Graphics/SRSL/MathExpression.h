//
// Created by Monika on 23.01.2023.
//

#ifndef SR_ENGINE_SRSL_MATHEXPRESSION_H
#define SR_ENGINE_SRSL_MATHEXPRESSION_H

#include <Utils/Common/Singleton.h>
#include <Graphics/SRSL/LexicalTree.h>

namespace SR_SRSL_NS {
    class SRSLMathExpression : public SR_UTILS_NS::Singleton<SRSLMathExpression> {
        SR_REGISTER_SINGLETON(SRSLMathExpression)
        struct ParseTokenStackData {
            std::string_view value;
            ~ParseTokenStackData();
        };
    public:
        SR_NODISCARD std::pair<SRSLExpr*, SRSLResult> Analyze(std::vector<Lexem>&& lexems);

    private:
        void Clear();

        SR_NODISCARD int32_t GetPriority(const std::string_view& operation, bool prefix) const;
        SR_NODISCARD bool IsIncrementOrDecrement(const std::string_view& operation) const;

        SR_NODISCARD SRSLExpr* ParseBinaryExpression(int32_t minPriority);
        SR_NODISCARD SRSLExpr* ParseSimpleExpression();
        SR_NODISCARD SRSLExpr* TryParseString();

        void ParseToken(std::string& token);
        ParseTokenStackData ParseTokenStack();
        void PopTokenStack();

        SR_NODISCARD bool IsPrefix() const noexcept;

        SR_NODISCARD bool InBounds() const noexcept;
        SR_NODISCARD bool IsHasErrors() const noexcept;
        SR_NODISCARD const Lexem* GetLexem(int64_t offset) const;
        SR_NODISCARD const Lexem* GetCurrentLexem() const;

    private:
        SRSLResult m_result;
        std::string m_tokenBufferTmp;
        std::string m_tryParseStringTokenTmp;

        std::array<std::string, 64> m_tokenStack;
        uint32_t m_tokenStackSize = 0;

        std::vector<Lexem> m_lexems;
        int64_t m_currentLexem = 0;

    };
}

#endif //SR_ENGINE_SRSL_MATHEXPRESSION_H
