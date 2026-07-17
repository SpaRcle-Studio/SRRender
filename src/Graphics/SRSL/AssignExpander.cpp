//
// Created by Monika on 04.02.2023.
//

#include <Graphics/SRSL/AssignExpander.h>
#include <Graphics/SRSL/MathExpression.h>

namespace SR_SRSL_NS {
    std::pair<std::vector<Lexem>, SRSLResult> SRSLAssignExpander::Expand(std::vector<Lexem>&& lexems) {
        Clear();

        m_lexems = SR_UTILS_NS::Exchange(lexems, { });

        while (InBounds() && !IsHasErrors()) {
            ProcessMain();
        }

        return std::make_pair(SR_UTILS_NS::Exchange(m_lexems, { }), std::move(m_result));
    }

    void SRSLAssignExpander::Clear() {
        m_currentLexem = 0;
        m_lexems.clear();
        m_result = SRSLResult();
    }

    const Lexem* SRSLAssignExpander::GetLexem(int64_t offset) const {
        if (m_currentLexem + offset < static_cast<int64_t>(m_lexems.size()) && m_currentLexem + offset >= 0) {
            return &m_lexems.at(m_currentLexem + offset);
        }

        return nullptr;
    }

    bool SRSLAssignExpander::InBounds() const noexcept {
        return m_currentLexem < m_lexems.size();
    }

    bool SRSLAssignExpander::IsHasErrors() const noexcept {
        return m_result.HasErrors();
    }

    const Lexem* SRSLAssignExpander::GetCurrentLexem() const {
        return GetLexem(0);
    }

    void SRSLAssignExpander::ProcessMain() {
        switch (m_lexems[m_currentLexem].kind) {
            case LexemKind::Plus:
            case LexemKind::Minus:
            case LexemKind::Multiply:
            case LexemKind::Divide:
            case LexemKind::Tilda:
            case LexemKind::Percent:
            case LexemKind::Exponentiation: {
                if (auto&& pNext = GetLexem(1); pNext && pNext->kind == LexemKind::Assign) {
                    ExpandDouble();
                    return;
                }
                ++m_currentLexem;
                break;
            }

            case LexemKind::OpeningAngleBracket: {
                if (auto&& pNext = GetLexem(1); pNext && pNext->kind == LexemKind::OpeningAngleBracket) {
                    if (auto&& pNextNext = GetLexem(2); pNextNext && pNextNext->kind == LexemKind::Assign) {
                        ExpandTriple();
                        return;
                    }
                }
                ++m_currentLexem;
                break;
            }

            case LexemKind::ClosingAngleBracket: {
                if (auto&& pNext = GetLexem(1); pNext && pNext->kind == LexemKind::ClosingAngleBracket) {
                    if (auto&& pNextNext = GetLexem(2); pNextNext && pNextNext->kind == LexemKind::Assign) {
                        ExpandTriple();
                        return;
                    }
                }
                ++m_currentLexem;
                break;
            }

            default:
                ++m_currentLexem;
                break;
        }
    }

    void SRSLAssignExpander::ExpandDouble() {
        auto&& lexems = GetLeftSide();

        if (lexems.empty()) {
            m_result.AddError(SRSLReturnCode::UnexceptedError);
            SRHalt0();
            return;
        }

        --m_currentLexem; /// back to variable name

        if (!GetLexem(0) || !GetLexem(1) || !GetLexem(2)) {
            m_result.AddError(SRSLReturnCode::InvalidExpression);
            return;
        }

        Lexem variable = *GetLexem(0);
        Lexem operation = *GetLexem(1);
        Lexem assign = *GetLexem(2);

        SR_UTILS_NS::Vector<Lexem> copy;
        copy.insert(copy.end(), m_lexems.begin() + m_currentLexem, m_lexems.end());
        auto&& [pExpr, result] = SRSLMathExpression::Instance().Analyze(std::move(copy));

        if (result.HasErrors()) {
            m_result.AddError(SRSLReturnCode::InvalidExpression);
            return;
        }

        /// variable = (variable operation expression)
        //m_lexems.insert(m_lexems.begin() + m_currentLexem, variable);
        //m_lexems.insert(m_lexems.begin() + m_currentLexem + 1, assign);
        //m_lexems.insert(m_lexems.begin() + m_currentLexem + 2, Lexem(variable.offset, 1, LexemKind::OpeningBracket, "(", variable.fileIndex, variable.line, variable.position));
        //m_lexems.insert(m_lexems.begin() + m_currentLexem + 3, variable);
        //m_lexems.insert(m_lexems.begin() + m_currentLexem + 4, operation);
        //m_lexems.insert(m_lexems.begin() + m_currentLexem + 5 + result.processedLexems, Lexem(variable.offset, 1, LexemKind::ClosingBracket, ")", variable.fileIndex, variable.line, variable.position));

        /// add closing bracket to end expression
        m_lexems.insert(m_lexems.begin() + m_currentLexem + result.processedLexems, Lexem(variable.offset, 1, LexemKind::ClosingBracket, ")", variable.fileIndex, variable.line, variable.position));
        /// add opening bracket to expression
        m_lexems.insert(m_lexems.begin() + m_currentLexem + 2, Lexem(variable.offset, 1, LexemKind::OpeningBracket, "(", variable.fileIndex, variable.line, variable.position));
        /// add variable name af
        m_lexems.insert(m_lexems.begin() + m_currentLexem + 3, variable);


        //m_currentLexem += 5 + result.processedLexems;
        ++m_currentLexem; /// closing bracket
    }

    void SRSLAssignExpander::ExpandTriple() {
        m_result.AddError(SRSLReturnCode::UnexceptedError);
        SRHalt("TODO!");
    }

    std::vector<Lexem> SRSLAssignExpander::GetLeftSide() {
        int32_t offset = -1;
        std::vector<Lexem> lexems;

        while (auto&& pLexem = GetLexem(offset)) {
            switch (pLexem->kind) {
                case LexemKind::Unknown:
                case LexemKind::OpeningCurlyBracket:
                case LexemKind::ClosingCurlyBracket:
                case LexemKind::Semicolon:
                    goto exit;
                default:
                    lexems.emplace(lexems.begin(), *pLexem);
                    --offset;
                    break;
            }
        }

    exit:
        return lexems;
    }

    uint64_t SRSLAssignExpander::FindSemicolon() {
        int64_t offset = 0;
        while (auto&& pLexem = GetLexem(offset)) {
            if (pLexem->kind == LexemKind::Semicolon) {
                return m_currentLexem + offset;
            }
            ++offset;
        }
        return SR_ID_INVALID;
    }

    uint64_t SRSLAssignExpander::FindClosingBracket() {
        int64_t offset = 0;
        while (auto&& pLexem = GetLexem(offset)) {
            if (pLexem->kind == LexemKind::ClosingBracket) {
                return m_currentLexem + offset;
            }
            ++offset;
        }
        return SR_ID_INVALID;
    }
}
