//
// Created by Monika on 24.01.2023.
//

#ifndef SR_ENGINE_SRSL_ASSIGNEXPANDER_H
#define SR_ENGINE_SRSL_ASSIGNEXPANDER_H

#include <Graphics/SRSL/LexicalTree.h>

#include <Utils/Common/Singleton.h>
#include <Utils/Lexer/LexerUtils.h>

namespace SR_SRSL_NS {
    class SRSLAssignExpander : public SR_UTILS_NS::Singleton<SRSLAssignExpander> {
        SR_REGISTER_SINGLETON(SRSLAssignExpander)
    public:
        SR_NODISCARD std::pair<std::vector<SR_UTILS_NS::LexerDetails::Lexem>, SR_UTILS_NS::LexerDetails::LexerResult> Expand(SR_UTILS_NS::IAllocator* pAllocator, std::vector<SR_UTILS_NS::LexerDetails::Lexem>&& lexems);

    private:
        void Clear();

        SR_NODISCARD std::vector<SR_UTILS_NS::LexerDetails::Lexem> GetLeftSide();
        SR_NODISCARD uint64_t FindSemicolon();
        SR_NODISCARD uint64_t FindClosingBracket();

        void ProcessMain();
        void ExpandDouble();
        void ExpandTriple();

        SR_NODISCARD bool InBounds() const noexcept;
        SR_NODISCARD bool IsHasErrors() const noexcept;
        SR_NODISCARD const SR_UTILS_NS::LexerDetails::Lexem* GetLexem(int64_t offset) const;
        SR_NODISCARD const SR_UTILS_NS::LexerDetails::Lexem* GetCurrentLexem() const;

    private:
        SR_UTILS_NS::LexerDetails::LexerResult m_result;
        SR_UTILS_NS::IAllocator* m_pAllocator = nullptr;

        std::vector<SR_UTILS_NS::LexerDetails::Lexem> m_lexems;
        int64_t m_currentLexem = 0;

    };
}

#endif //SR_ENGINE_SRSL_ASSIGNEXPANDER_H
