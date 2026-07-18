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
        SR_NODISCARD std::pair<SRSLExpr*, SRSLResult> Analyze(SR_UTILS_NS::IAllocator* pAllocator, std::span<Lexem> lexems);

    private:
        void Clear();

        SR_NODISCARD int32_t GetPriority(const std::string_view& operation, bool prefix) const;
        SR_NODISCARD bool IsIncrementOrDecrement(const std::string_view& operation) const;

        SR_NODISCARD SRSLExpr* ParseBinaryExpression(int32_t minPriority);
        SR_NODISCARD SRSLExpr* ParseSimpleExpression();
        SR_NODISCARD SRSLExpr* TryParseString();

        void ParseToken(SR_UTILS_NS::String& token);
        ParseTokenStackData ParseTokenStack();
        void PopTokenStack();

        SR_NODISCARD bool IsPrefix() const noexcept;

        SR_NODISCARD bool InBounds() const noexcept;
        SR_NODISCARD bool IsHasErrors() const noexcept;
        SR_NODISCARD const Lexem* GetLexem(int64_t offset) const;
        SR_NODISCARD const Lexem* GetCurrentLexem() const;

    private:
        SR_UTILS_NS::IAllocator* m_pAllocator = nullptr;
        SRSLResult m_result;
        SR_UTILS_NS::String m_tokenBufferTmp;
        SR_UTILS_NS::String m_tryParseStringTokenTmp;

        std::array<SR_UTILS_NS::String, 64> m_tokenStack;
        uint32_t m_tokenStackSize = 0;

        std::array<SR_UTILS_NS::Vector<Lexem>, 32> m_bracketLexems;
        uint32_t m_bracketLexemsSize = 0;

        std::span<Lexem> m_lexems;
        int64_t m_currentLexem = 0;

    };
}

#endif //SR_ENGINE_SRSL_MATHEXPRESSION_H
