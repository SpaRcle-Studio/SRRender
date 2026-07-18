//
// Created by Monika on 23.01.2023.
//

#include <Graphics/SRSL/MathExpression.h>

namespace SR_SRSL_NS {
    std::pair<SRSLExpr*, SRSLResult> SRSLMathExpression::Analyze(SR_UTILS_NS::IAllocator* pAllocator, std::span<Lexem> lexems) {
        SR_TRACY_ZONE;

        Clear();

        m_pAllocator = pAllocator;
        m_lexems = SR_UTILS_NS::Exchange(lexems, { });

        if (m_lexems.empty()) {
            return std::make_pair(nullptr, SRSLReturnCode::EmptyExpression);
        }

        auto&& pExpr = ParseBinaryExpression(0);
        m_result.processedLexems = m_currentLexem;
        return std::make_pair(pExpr, std::move(m_result));
    }

    SRSLExpr* SRSLMathExpression::ParseBinaryExpression(int32_t minPriority) {
        SRSLExpr* pLeftExpr = ParseSimpleExpression();

        if (!InBounds()) {
            return pLeftExpr;
        }

        while (true) {
            uint32_t currentLexem = m_currentLexem;
            ParseTokenStackData operation = ParseTokenStack();

            if (IsHasErrors()) {
                return nullptr;
            }

            if (minPriority != 0 && operation.value == ".") {
                m_currentLexem = currentLexem;
                return pLeftExpr;
            }

            if (operation.value == "]" || operation.value == ",") {
                return pLeftExpr;
            }

            const int32_t priority = GetPriority(operation.value, IsPrefix());

            if (priority <= minPriority) {
                m_currentLexem = currentLexem;
                return pLeftExpr;
            }

            if (IsIncrementOrDecrement(operation.value)) {
                /// постинкремент
                pLeftExpr = AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, pLeftExpr, AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, operation.value));

                if (!InBounds()) {
                    return pLeftExpr;
                }

                currentLexem = m_currentLexem;
                ParseToken(m_tokenBufferTmp);
                if (m_tokenBufferTmp == ")") {
                    m_currentLexem = currentLexem;
                    return pLeftExpr;
                }
                m_currentLexem = currentLexem;

                auto&& pRightExpr = ParseBinaryExpression(priority);

                /// лексемы закончились
                if (!pRightExpr) {
                    return pLeftExpr;
                }

                if (pRightExpr->args.size() != 1) {
                    m_result = SRSLResult(SRSLReturnCode::InvalidIncrementOrDecrement);
                    return pLeftExpr;
                }
                else {
                    pLeftExpr = AllocateLexicalUnit<SRSLExpr>(*m_pAllocator,
                        std::move(pRightExpr->token),
                        pLeftExpr,
                        SR_UTILS_NS::Exchange(pRightExpr->args[0], nullptr)
                    );
                }
            }
            else {
                if (InBounds()) {
                    auto&& pRightExpr = ParseBinaryExpression(priority);
                    pLeftExpr = AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, operation.value, pLeftExpr, pRightExpr);
                }
                else {
                    return pLeftExpr;
                }
            }

            if (!InBounds() || GetCurrentLexem()->kind == LexemKind::OpeningSquareBracket || GetLexem(-1)->kind == LexemKind::Comma) {
                return pLeftExpr;
            }
        }
    }

    SRSLExpr* SRSLMathExpression::ParseSimpleExpression() {
        if (auto&& pExpr = TryParseString()) {
            return pExpr;
        }

        ParseTokenStackData token = ParseTokenStack();

        if (IsHasErrors()) {
            return nullptr;
        }

        if (token.value.empty()) {
            m_result = SRSLResult(SRSLReturnCode::EmptyToken);
            return nullptr;
        }

        if (SR_MATH_NS::IsNumber(token.value) || IsIdentifier(token.value)) {
            auto&& pBasicExpr = AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, token.value);

            /// parse function call
            if (auto&& pLexem = GetCurrentLexem(); pLexem && pLexem->kind == LexemKind::OpeningBracket) {
                ++m_currentLexem;

                pBasicExpr->isCall = true;

            retryFnArg:
                pLexem = GetCurrentLexem();
                if (!pLexem || IsHasErrors()) {
                    m_result = SRSLResult(SRSLReturnCode::InvalidCall);
                    return nullptr;
                }

                if (pLexem->kind == LexemKind::Comma) {
                    ++m_currentLexem;
                    goto retryFnArg;
                }

                if (pLexem->kind == LexemKind::ClosingBracket) {
                    ++m_currentLexem;
                    return pBasicExpr;
                }

                auto&& pArgExpr = ParseBinaryExpression(0);
                if (pArgExpr) {
                    pBasicExpr->args.emplace_back(pArgExpr);
                }

                goto retryFnArg;
            }

        retrySubExpr:
            if (auto&& pLexem = GetCurrentLexem(); pLexem && pLexem->kind == LexemKind::OpeningSquareBracket) {
                ++m_currentLexem;

                if (m_bracketLexemsSize >= m_bracketLexems.size()) {
                    SRHalt("SRSLMathExpression::ParseSimpleExpression() : too many nested brackets!");
                    return nullptr;
                }

                auto&& bracketLexems = m_bracketLexems[m_bracketLexemsSize++];
                bracketLexems.clear();
                bracketLexems.reserve(16);

                int32_t bracketCount = 1;

                while (InBounds() && bracketCount > 0) {
                    auto&& pNextLexem = GetCurrentLexem();
                    if (pNextLexem->kind == LexemKind::OpeningSquareBracket) {
                        ++bracketCount;
                    }
                    else if (pNextLexem->kind == LexemKind::ClosingSquareBracket) {
                        --bracketCount;
                        if (bracketCount == 0) {
                            ++m_currentLexem;
                            break;
                        }
                    }
                    bracketLexems.emplace_back(*pNextLexem);
                    ++m_currentLexem;
                }

                if (bracketLexems.empty()) {
                    pBasicExpr = AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, "[", pBasicExpr, nullptr);
                }
                else {
                    const int64_t stashLexem = m_currentLexem;
                    std::span<Lexem> oldLexems = m_lexems;

                    auto&& pInnerExpr = SRSLMathExpression::Instance().Analyze(m_pAllocator, bracketLexems);
                    if (pInnerExpr.second.HasErrors()) {
                        m_result = pInnerExpr.second;
                        SR_ERROR("SRSLMathExpression::ParseSimpleExpression() : failed to parse inner expression!");
                        m_bracketLexemsSize--;
                        return nullptr;
                    }

                    m_currentLexem = stashLexem;
                    m_lexems = oldLexems;

                    pBasicExpr = AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, "[", pBasicExpr, pInnerExpr.first);
                }
                m_bracketLexemsSize--;

                //if (auto&& pNextLexem = GetCurrentLexem(); pNextLexem && pNextLexem->kind == LexemKind::ClosingSquareBracket) {
                //    ++m_currentLexem;
                //    pBasicExpr = new SRSLExpr("[", pBasicExpr, nullptr);
                //    goto retrySubExpr;
                //}

                //auto&& pExpr = ParseBinaryExpression(30 /** = */);

                goto retrySubExpr;
            }
            else if (pLexem && pLexem->kind == LexemKind::Dot) {
                ++m_currentLexem;
                std::string_view field = GetCurrentLexem()->value;
                auto&& pExpr = AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, field);
                pBasicExpr = AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, ".", pBasicExpr, pExpr);
                ++m_currentLexem;
                goto retrySubExpr;
            }
            else if (pLexem && pLexem->kind == LexemKind::Identifier && pLexem->value.starts_with('x')) {
                if (pBasicExpr->token == "0") {
                    pBasicExpr->token += pLexem->value;
                    ++m_currentLexem;
                }
            }
            else if (pLexem && pLexem->kind == LexemKind::Identifier && pLexem->value.starts_with('u')) {
                if (SR_MATH_NS::IsNumber(pBasicExpr->token)) {
                    pBasicExpr->token += pLexem->value;
                    ++m_currentLexem;
                }
            }
            else if (pLexem && pLexem->kind == LexemKind::Identifier && pLexem->value.starts_with('e')) {
                if (SR_MATH_NS::IsNumber(pBasicExpr->token)) {
                    pBasicExpr->token += pLexem->value;
                    ++m_currentLexem;
                    if (auto&& pNextMinusLexem = GetCurrentLexem(); pNextMinusLexem && pNextMinusLexem->kind == LexemKind::Minus) {
                        pBasicExpr->token += pNextMinusLexem->value;
                        ++m_currentLexem;
                        while (InBounds() && SR_MATH_NS::IsNumber(GetCurrentLexem()->value)) {
                            pBasicExpr->token += GetCurrentLexem()->value;
                            ++m_currentLexem;
                        }
                    }
                }
            }

            return pBasicExpr;
        }

        if (token.value.size() == 1 && (token.value == "(")) {
            auto&& pExpr = ParseBinaryExpression(0);

            if (!InBounds()) {
                m_result = SRSLResult(SRSLReturnCode::InvalidComplexExpression, GetCurrentLexem());
                return nullptr;
            }

            ParseToken(m_tokenBufferTmp);
            if (m_tokenBufferTmp != ")") {
                m_result = SRSLResult(SRSLReturnCode::InvalidComplexExpression, GetCurrentLexem());
                return nullptr;
            }

            return pExpr;
        }

        /// parse list { ... }
        if (token.value.size() == 1 && token.value == "{") {
            auto&& pListExpr = AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, token.value);

        labelNextArrayElem:
            int64_t currentLexemStash = m_currentLexem;

            token = ParseTokenStack();

            if (token.value.empty()) {
                m_result = SRSLResult(SRSLReturnCode::InvalidListEnd);
                return nullptr;
            }

            if (token.value == ",") {
                goto labelNextArrayElem;
            }

            if (token.value == "}") {
                return pListExpr;
            }

            m_currentLexem = currentLexemStash;

            if (auto&& pListElemExpr = ParseBinaryExpression(0)) {
                pListExpr->args.emplace_back(pListElemExpr);
            }

            goto labelNextArrayElem;
        }

        if (!InBounds()) {
            return AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, token.value);
        }

        auto&& pArgExpr = ParseSimpleExpression();

        if (IsHasErrors()) {
            return nullptr;
        }

        if (IsIncrementOrDecrement(token.value)) {
            return AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, token.value), pArgExpr);
        }

        return AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, token.value, pArgExpr);
    }

    SRSLExpr* SRSLMathExpression::TryParseString() {
        bool isStringStarted = false;
        m_tryParseStringTokenTmp.clear();

    retry:
        if (!InBounds()) {
            return nullptr;
        }

        switch (GetCurrentLexem()->kind) {
            case LexemKind::String: {
                ++m_currentLexem;
                if (isStringStarted) {
                    return SRSLExpr::CreateStringExpression(*m_pAllocator, m_tryParseStringTokenTmp);
                }
                isStringStarted = true;
                goto retry;
            }
            default: {
                if (!isStringStarted) {
                    return nullptr;
                }

                if (GetCurrentLexem()->value.empty()) {
                    if (std::string str = LexemKindToString(GetCurrentLexem()->kind); str.empty()) {
                        SRHalt("Unknown lexem kind and empty value!");
                    }
                    else {
                        m_tryParseStringTokenTmp += str;
                    }
                }
                else {
                    if (GetCurrentLexem()->value == "\"" && GetLexem(1) && GetLexem(1)->kind == LexemKind::String) {
                        ++m_currentLexem;
                        m_tryParseStringTokenTmp += "\"";
                    }
                    else {
                        m_tryParseStringTokenTmp += GetCurrentLexem()->value;
                    }
                }
                ++m_currentLexem;
                goto retry;
            }
        }
    }

    int32_t SRSLMathExpression::GetPriority(const std::string_view& operation, bool prefix) const {
        if (operation.size() == 1) {
            const char op = operation[0];

            if (prefix) {
                switch(op) {
                    case '~': return 35;
                    case '!': return 40;
                    case '+':
                    case '-': return 45;
                    default:
                        break;
                }
            }

            switch (op) {
                case '=': return 30;
                case '[': return 50;
                case ']': return 50;

                case '?': return 60;
                case ':': return 60;
                case '|': return 75;
                case '^': return 76;
                case '&': return 77;

                case '+': return 100;
                case '-': return 100;
                case '>': return 85;
                case '<': return 85;
                case '*': return 300;
                case '/': return 300;
                case '%': return 300;
                case '.': return 600;
                default:
                    break;
            }

            return 0;
        }
        else if (operation.size() == 2) {
            if (operation == "+=") return 10;
            else if (operation == "-=") return 10;
            else if (operation == "%=") return 10;
            else if (operation == "|=") return 10;
            else if (operation == "&=") return 10;
            else if (operation == "^=") return 10;
            else if (operation == "~=") return 10;
            else if (operation == "*=") return 10;
            else if (operation == "/=") return 10;
            else if (operation == "||") return 71;
            else if (operation == "^^") return 72;
            else if (operation == "&&") return 73;

            else if (operation == "!=") return 80;
            else if (operation == "==") return 80;

            else if (operation == ">=") return 85;
            else if (operation == "<=") return 85;

            else if (operation == ">>") return 90;
            else if (operation == "<<") return 90;

            else if (operation == "++") return 500;
            else if (operation == "--") return 500;

            return 0;
        }
        else if (operation == ">>=" || operation == "<<=") {
            return 10;
        }

        return 0;
    }

    void SRSLMathExpression::Clear() {
        m_result.Clear();
        m_currentLexem = 0;
    }

    const Lexem* SRSLMathExpression::GetLexem(int64_t offset) const {
        if (m_currentLexem + offset < static_cast<int64_t>(m_lexems.size())) {
            return &m_lexems[m_currentLexem + offset];
        }

        return nullptr;
    }

    bool SRSLMathExpression::InBounds() const noexcept {
        return m_currentLexem < m_lexems.size();
    }

    bool SRSLMathExpression::IsHasErrors() const noexcept {
        return m_result.HasErrors();
    }

    const Lexem* SRSLMathExpression::GetCurrentLexem() const {
        return GetLexem(0);
    }

    void SRSLMathExpression::ParseToken(SR_UTILS_NS::String& token) {
        token.clear();

        /// пытаемся обработать как число
        {
            bool hasDot = false;
            bool hasInt = false;

        retry:
            switch (InBounds() ? GetCurrentLexem()->kind : LexemKind::Unknown) {
                case LexemKind::Integer:
                    token += m_lexems[m_currentLexem++].value;
                    hasInt = true;
                    goto retry;
                case LexemKind::Dot:
                    if (hasDot) {
                        m_result = SRSLResult(SRSLReturnCode::UnexceptedDot, GetCurrentLexem());
                        token.clear();
                        return;
                    }
                    token += m_lexems[m_currentLexem++].value;
                    hasDot = true;
                    goto retry;
                default: {
                    if (hasDot && !hasInt) {
                        return;
                    }
                    else if (hasInt) {
                        return;
                    }

                    break;
                }
            }
        }

        token.clear();

        if (!GetCurrentLexem()) {
            m_result = SRSLResult(SRSLReturnCode::EmptyToken);
            SRHalt("SRSLMathExpression::ParseToken() : GetCurrentLexem() is nullptr!");
            return;
        }

        switch (GetCurrentLexem()->kind) {
            case LexemKind::OpeningBracket:
            case LexemKind::ClosingBracket:
            case LexemKind::OpeningSquareBracket:
            case LexemKind::ClosingSquareBracket:
            case LexemKind::ClosingCurlyBracket:
            case LexemKind::OpeningCurlyBracket:
            case LexemKind::Identifier:
            case LexemKind::Tilda:
            case LexemKind::Comma:
            case LexemKind::Dot:
            case LexemKind::Question:
            case LexemKind::Colon: {
                token = m_lexems[m_currentLexem++].value;
                return;
            }

            case LexemKind::Negation: {
            case LexemKind::Multiply:
            case LexemKind::Divide:
            case LexemKind::Percent:
            case LexemKind::Assign:
                if (auto&& pNext = GetLexem(1); pNext && pNext->kind == LexemKind::Assign) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// !=
                }
                token = m_lexems[m_currentLexem++].value;
                return; /// !
            }

            case LexemKind::Exponentiation: {
                if (auto&& pNext = GetLexem(1); pNext && pNext->kind == LexemKind::Exponentiation) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// ^^
                }
                else if (pNext && pNext->kind == LexemKind::Assign) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// ^=
                }
                token = m_lexems[m_currentLexem++].value; /// ^
                return;
            }

            case LexemKind::OpeningAngleBracket: {
                if (auto&& pNext = GetLexem(1); pNext && pNext->kind == LexemKind::Assign) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// <=
                }
                else if (pNext && pNext->kind == LexemKind::OpeningAngleBracket) {
                    if (auto&& pNextNext = GetLexem(2); pNextNext && pNextNext->kind == LexemKind::Assign) {
                        token += m_lexems[m_currentLexem].value;
                        token += m_lexems[m_currentLexem + 1].value;
                        token += m_lexems[m_currentLexem + 2].value;
                        m_currentLexem += 3;
                        return; /// <<=
                    }
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// <<
                }
                token = m_lexems[m_currentLexem++].value; /// <
                return;
            }

            case LexemKind::ClosingAngleBracket: {
                if (auto&& pNext = GetLexem(1); pNext && pNext->kind == LexemKind::Assign) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// >=
                }
                else if (pNext && pNext->kind == LexemKind::ClosingAngleBracket) {
                    if (auto&& pNextNext = GetLexem(2); pNextNext && pNextNext->kind == LexemKind::Assign) {
                        token += m_lexems[m_currentLexem].value;
                        token += m_lexems[m_currentLexem + 1].value;
                        token += m_lexems[m_currentLexem + 2].value;
                        m_currentLexem += 3;
                        return; /// >>=
                    }
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// >>
                }
                token = m_lexems[m_currentLexem++].value; /// >
                return;
            }

            case LexemKind::Plus: {
                if (auto&& pNext = GetLexem(1); pNext && pNext->kind == LexemKind::Assign) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// +=
                }
                else if (pNext && pNext->kind == LexemKind::Plus) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// ++
                }
                token = m_lexems[m_currentLexem++].value; /// +
                return;
            }

            case LexemKind::Minus: {
                if (auto&& pNext = GetLexem(1); pNext && pNext->kind == LexemKind::Assign) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// -=
                }
                else if (pNext && pNext->kind == LexemKind::Minus) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// --
                }
                token = m_lexems[m_currentLexem++].value; /// +
                return;
            }

            case LexemKind::And: {
                if (auto&& pNext = GetLexem(1); pNext && pNext->kind == LexemKind::And) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// &&
                }
                else if (pNext && pNext->kind == LexemKind::Assign) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// &=
                }
                token = m_lexems[m_currentLexem++].value; /// &
                return;
            }

            case LexemKind::Or: {
                if (auto&& pNext = GetLexem(1); pNext && pNext->kind == LexemKind::Or) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// ||
                }
                else if (pNext && pNext->kind == LexemKind::Assign) {
                    token += m_lexems[m_currentLexem].value;
                    token += m_lexems[m_currentLexem + 1].value;
                    m_currentLexem += 2;
                    return; /// |=
                }
                token = m_lexems[m_currentLexem++].value; /// |
                return;
            }

            default:
                break;
        }

        m_result = SRSLResult(SRSLReturnCode::InvalidMathToken, GetCurrentLexem());
        token.clear();
    }

    bool SRSLMathExpression::IsIncrementOrDecrement(const std::string_view& operation) const {
        return operation == "++" || operation == "--";
    }

    bool SRSLMathExpression::IsPrefix() const noexcept {
        auto&& pLexem = GetLexem(-1);

        if (!pLexem) {
            return true;
        }

        if (IsOperator(pLexem->value)) {
            return false;
        }

        return pLexem->kind != LexemKind::Identifier;
    }

    void SRSLMathExpression::PopTokenStack() {
        if (m_tokenStackSize > 0) {
            m_tokenStackSize--;
        }
        else {
            SRHalt("SRSLMathExpression::PopTokenStack() : token stack is empty!");
        }
    }

    SRSLMathExpression::ParseTokenStackData SRSLMathExpression::ParseTokenStack() {
        if (m_tokenStackSize >= m_tokenStack.size()) {
            SRHalt("SRSLMathExpression::ParseTokenStack() : token stack overflow!");
            return { };
        }
        SR_UTILS_NS::String& data = m_tokenStack[m_tokenStackSize++];
        ParseToken(data);
        return { data };
    }

    SRSLMathExpression::ParseTokenStackData::~ParseTokenStackData() {
        SRSLMathExpression::Instance().PopTokenStack();
    }
}