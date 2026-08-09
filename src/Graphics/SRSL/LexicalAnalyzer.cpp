//
// Created by Monika on 22.01.2023.
//

#include <Graphics/SRSL/LexicalAnalyzer.h>

namespace SR_SRSL_NS {
    using SRSLResult = SR_UTILS_NS::LexerDetails::LexerResult;
    using SRSLReturnCode = SR_UTILS_NS::LexerDetails::LexerReturnCode;
    using Lexem = SR_UTILS_NS::LexerDetails::Lexem;
    using LexemKind = SR_UTILS_NS::LexerDetails::LexemKind;

    std::pair<SRSLAnalyzedTree*, SRSLResult> SRSLLexicalAnalyzer::Analyze(SR_UTILS_NS::IAllocator* pAllocator, std::span<Lexem> lexems) {
        SR_TRACY_ZONE;
        SR_GLOBAL_LOCK

        Clear();

        m_lexems = lexems;

        auto&& pAnalyzedTree = AllocateLexicalUnit<SRSLAnalyzedTree>(*pAllocator);
        m_pAllocator = pAllocator;

        ProcessMain();

        if (IsHasErrors()) {
            return std::make_pair(nullptr, SR_UTILS_NS::Exchange(m_result, { }));
        }

        if (m_lexicalTree.size() != 1) {
            return std::make_pair(nullptr, SR_SRSL_NS::SRSLResult(SRSLReturnCode::InvalidLexicalTree));
        }

        pAnalyzedTree->pLexicalTree = SR_UTILS_NS::Exchange(*m_lexicalTree.begin(), { });

        return std::make_pair(std::move(pAnalyzedTree), SR_UTILS_NS::Exchange(m_result, { }));
    }

    void SRSLLexicalAnalyzer::Clear() {
        SR_TRACY_ZONE;

        m_lexicalTree.clear();
        m_currentLexem = 0;

        m_states.clear();
        m_result = SRSLResult();
    }

    const Lexem *SRSLLexicalAnalyzer::GetLexem(int64_t offset) const {
        if (m_currentLexem + offset < static_cast<int64_t>(m_lexems.size())) {
            return &m_lexems[m_currentLexem + offset];
        }
        return nullptr;
    }

    bool SRSLLexicalAnalyzer::InBounds() const noexcept {
        return m_currentLexem < m_lexems.size();
    }

    void SRSLLexicalAnalyzer::ProcessMain() {
        SR_TRACY_ZONE;

        m_lexicalTree.emplace_back(AllocateLexicalUnit<SRSLLexicalTree>(*m_pAllocator));

        while (InBounds() && !IsHasErrors()) {
            switch (m_lexems[m_currentLexem].kind) {
                case LexemKind::OpeningSquareBracket:
                case LexemKind::ClosingSquareBracket:
                case LexemKind::OpeningAngleBracket:
                case LexemKind::ClosingAngleBracket:
                case LexemKind::OpeningCurlyBracket:
                case LexemKind::ClosingCurlyBracket:
                case LexemKind::OpeningBracket:
                case LexemKind::ClosingBracket: {
                    ProcessBracket();
                    if (IsHasErrors()) {
                        return;
                    }
                    break;
                }

                case LexemKind::Identifier: {
                    if (ProcessInBuiltName()) {
                        break;
                    }

                    if (auto&& pUnit = TryProcessIdentifier()) {
                        if (dynamic_cast<SRSLFunction*>(pUnit)) {
                            m_states.emplace_back(LXAState::Function);
                            m_lexicalTree.back()->lexicalTree.emplace_back(pUnit);
                        }
                        else if (!m_states.empty() && m_states.back() == LXAState::ForStatementVariable) {
                            auto&& pForStatement = dynamic_cast<SRSLForStatement*>(m_lexicalTree.back()->lexicalTree.back());
                            auto&& pVar = dynamic_cast<SRSLVariable*>(pUnit);
                            SRAssert(pForStatement && pVar);
                            if (!pForStatement || !pVar) {
                                return;
                            }
                            pForStatement->pVar = pVar;
                        }
                        else if (!m_states.empty() && m_states.back() == LXAState::FunctionArgs) {
                            auto&& pFunction = dynamic_cast<SRSLFunction*>(m_lexicalTree.back()->lexicalTree.back());
                            auto&& pVar = dynamic_cast<SRSLVariable*>(pUnit);
                            SRAssert(pFunction && pVar);
                            if (!pFunction || !pVar) {
                                return;
                            }
                            pFunction->args.emplace_back(pVar);
                        }
                        else {
                            m_lexicalTree.back()->lexicalTree.emplace_back(pUnit);
                        }
                        break;
                    }
                    if (IsHasErrors()) {
                        return;
                    }

                    SR_FALLTHROUGH;
                }
                case LexemKind::Plus:
                case LexemKind::Minus:
                case LexemKind::Tilda:
                case LexemKind::Integer:
                case LexemKind::Negation: {
                    ProcessExpression(false, true);
                    if (IsHasErrors()) {
                        return;
                    }
                    m_lexicalTree.back()->lexicalTree.emplace_back(SR_UTILS_NS::Exchange(m_expr, nullptr));
                    break;
                }

                case LexemKind::Assign:
                case LexemKind::Semicolon: {
                    if (!m_states.empty() && m_states.back() == LXAState::ForStatementVariable) {
                        m_states.back() = LXAState::ForStatementCondition;
                        ++m_currentLexem;

                        ProcessExpression(false, true);

                        auto&& pForStatement = dynamic_cast<SRSLForStatement*>(m_lexicalTree.back()->lexicalTree.back());
                        SRAssert(pForStatement && m_expr);
                        if (!pForStatement || !m_expr) {
                            return;
                        }

                        pForStatement->pCondition = SR_UTILS_NS::Exchange(m_expr, nullptr);
                        break;
                    }
                    else if (!m_states.empty() && m_states.back() == LXAState::ForStatementCondition) {
                        m_states.back() = LXAState::ForStatementExpression;
                        ++m_currentLexem;

                        ProcessExpression(false, true);

                        auto&& pForStatement = dynamic_cast<SRSLForStatement*>(m_lexicalTree.back()->lexicalTree.back());
                        SRAssert(pForStatement && m_expr);
                        if (!pForStatement || !m_expr) {
                            return;
                        }

                        pForStatement->pExpr = SR_UTILS_NS::Exchange(m_expr, nullptr);

                        SRAssert(GetCurrentLexem() && GetCurrentLexem()->kind == LexemKind::ClosingBracket);
                        ++m_currentLexem; /// )

                        break;
                    }
                    SR_FALLTHROUGH;
                }
                case LexemKind::Dot:
                case LexemKind::Comma:
                case LexemKind::And:
                case LexemKind::Or:
                case LexemKind::Macro:
                case LexemKind::MacroEnd:
                    ++m_currentLexem;
                    break;
                default:
                    m_result = SRSLResult(SRSLReturnCode::UnknownLexem, GetLexem(0));
                    break;
            }
        }
    }

    void SRSLLexicalAnalyzer::ProcessBracket() {
        SR_TRACY_ZONE;

        switch (m_lexems[m_currentLexem].kind)
        {
            case LexemKind::OpeningBracket: {
                if (!m_states.empty() && m_states.back() == LXAState::ForStatement) {
                    m_states.back() = LXAState::ForStatementVariable;
                    ++m_currentLexem;
                    return;
                }
                else if (!m_states.empty() && m_states.back() == LXAState::WhileStatement) {
                    m_states.back() = LXAState::WhileStatementCondition;
                    ++m_currentLexem;

                    ProcessExpression(false, true);

                    auto&& pWhileStatement = dynamic_cast<SRSLWhileStatement*>(m_lexicalTree.back()->lexicalTree.back());
                    SRAssert(pWhileStatement && m_expr);

                    pWhileStatement->pCondition = SR_UTILS_NS::Exchange(m_expr, nullptr);

                    SRAssert(GetCurrentLexem() && GetCurrentLexem()->kind == LexemKind::ClosingBracket);
                    ++m_currentLexem; /// )

                    return;
                }
                else if (!m_states.empty() && m_states.back() == LXAState::Function) {
                    ++m_currentLexem;
                    m_states.back() = LXAState::FunctionArgs;
                    return;
                }

                ProcessExpression();

                if (IsHasErrors()) {
                    return;
                }

                if (!m_states.empty() && m_states.back() == LXAState::IfStatement) {
                    auto&& pIfStatement = dynamic_cast<SRSLIfStatement*>(m_lexicalTree.back()->lexicalTree.back());
                    if (!pIfStatement) {
                        m_result = SRSLResult(SRSLReturnCode::InvalidIfStatement);
                        return;
                    }
                    pIfStatement->pExpr = SR_UTILS_NS::Exchange(m_expr, nullptr);
                }
                else {
                    m_lexicalTree.back()->lexicalTree.emplace_back(SR_UTILS_NS::Exchange(m_expr, nullptr));
                }

                return;
            }
            case LexemKind::ClosingBracket: {
                if (!m_states.empty() && m_states.back() == LXAState::FunctionArgs) {
                    m_states.back() = LXAState::Function;
                    ++m_currentLexem;
                    return;
                }

                break;
            }
            case LexemKind::OpeningSquareBracket: {
                ProcessDecorators();
                return;
            }
            case LexemKind::OpeningCurlyBracket: {
                m_lexicalTree.emplace_back(AllocateLexicalUnit<SRSLLexicalTree>(*m_pAllocator));
                if (m_lexicalTree.size() > 64 * 64 * 64) {
                    SR_ERROR("SRSLLexicalAnalyzer::ProcessBracket() : too deep nesting!");
                    ++m_currentLexem;
                    return;
                }

                if (!m_states.empty() && m_states.back() == LXAState::ForStatementExpression) {
                    m_states.back() = LXAState::ForStatementBody;
                    ++m_currentLexem;
                    return;
                }
                else if (!m_states.empty() && m_states.back() == LXAState::WhileStatementCondition) {
                    m_states.back() = LXAState::WhileStatementBody;
                    ++m_currentLexem;
                    return;
                }
                else if (!m_states.empty() && m_states.back() == LXAState::Function) {
                    m_states.back() = LXAState::FunctionBody;
                    ++m_currentLexem;
                    return;
                }
                else if (!m_states.empty() && m_states.back() == LXAState::IfStatement) {
                    m_states.back() = LXAState::IfStatementBody;
                    ++m_currentLexem;
                    return;
                }
                else if (!m_states.empty() && m_states.back() == LXAState::StructureStatement) {
                    m_states.back() = LXAState::StructureStatementBody;
                    ++m_currentLexem;
                    return;
                }

                return;
            }
            case LexemKind::ClosingCurlyBracket: {
                if (m_lexicalTree.size() <= 1) {
                    m_result = SRSLResult(SRSLReturnCode::InvalidScope, GetCurrentLexem());
                    return;
                }

                SRSLLexicalTree* pLexicalTree = m_lexicalTree.back();
                m_lexicalTree.pop_back();

                if (!m_states.empty() && m_states.back() == LXAState::FunctionBody) {
                    m_states.pop_back();

                    if (m_lexicalTree.back()->lexicalTree.empty()) {
                        m_result = SRSLResult(SRSLReturnCode::InvalidFunction, GetCurrentLexem());
                        return;
                    }

                    auto&& pFunction = dynamic_cast<SRSLFunction*>(m_lexicalTree.back()->lexicalTree.back());
                    if (!pFunction) {
                        m_result = SRSLResult(SRSLReturnCode::InvalidFunction, GetCurrentLexem());
                        return;
                    }
                    pFunction->pLexicalTree = std::move(pLexicalTree);
                }
                else if (!m_states.empty() && m_states.back() == LXAState::IfStatementBody) {
                    m_states.pop_back();
                    auto&& pIfStatement = dynamic_cast<SRSLIfStatement*>(m_lexicalTree.back()->lexicalTree.back());
                    pIfStatement->pLexicalTree = std::move(pLexicalTree);
                }
                else if (!m_states.empty() && m_states.back() == LXAState::ForStatementBody) {
                    m_states.pop_back();
                    auto&& pForStatement = dynamic_cast<SRSLForStatement*>(m_lexicalTree.back()->lexicalTree.back());
                    pForStatement->pLexicalTree = std::move(pLexicalTree);
                }
                else if (!m_states.empty() && m_states.back() == LXAState::WhileStatementBody) {
                    m_states.pop_back();
                    auto&& pWhileStatement = dynamic_cast<SRSLWhileStatement*>(m_lexicalTree.back()->lexicalTree.back());
                    pWhileStatement->pLexicalTree = std::move(pLexicalTree);
                }
                else if (!m_states.empty() && m_states.back() == LXAState::StructureStatementBody) {
                    m_states.pop_back();
                    auto&& pStructureStatement = dynamic_cast<SRSLStructureStatement*>(m_lexicalTree.back()->lexicalTree.back());
                    pStructureStatement->pLexicalTree = std::move(pLexicalTree);
                }
                else {
                    m_lexicalTree.back()->lexicalTree.emplace_back(pLexicalTree);
                }

                ++m_currentLexem;

                return;
            }
            default:
                break;
        }

        m_result = SRSLResult(SRSLReturnCode::UnexceptedLexem, GetCurrentLexem());
    }

    const Lexem* SRSLLexicalAnalyzer::GetCurrentLexem() const {
        return GetLexem(0);
    }

    void SRSLLexicalAnalyzer::ProcessExpression(bool isFunctionName, bool isSimpleExpr) {
        SR_TRACY_ZONE;

        m_exprLexems.clear();
        m_exprLexems.reserve(16);

        uint32_t deep = 0;
        bool allowIdentifier = true;

    retry:
        const LexemKind lexemKind = InBounds() ? m_lexems[m_currentLexem].kind : LexemKind::Unknown;
        switch (lexemKind)
        {
            case LexemKind::OpeningCurlyBracket:
                if (auto&& pPrev = GetLexem(-1)) {
                    if (pPrev->kind == LexemKind::Assign || pPrev->kind == LexemKind::OpeningCurlyBracket || pPrev->kind == LexemKind::Comma) {
                        m_exprLexems.emplace_back(m_lexems[m_currentLexem]);
                        ++m_currentLexem;
                        ++deep;
                        goto retry;
                    }
                }
                break;
            case LexemKind::OpeningBracket:
                if (isFunctionName && deep == 0) {
                    break;
                }
                SR_FALLTHROUGH;
            case LexemKind::OpeningSquareBracket:
                ++deep;
                SR_FALLTHROUGH;
            default: {
                switch (lexemKind) {
                    case LexemKind::Unknown:
                    case LexemKind::Semicolon:
                    case LexemKind::Macro:
                        break;

                    case LexemKind::Negation:
                        if (GetLexem(1) && GetLexem(1)->kind == LexemKind::Assign) {
                            m_exprLexems.emplace_back(m_lexems[m_currentLexem]);
                            ++m_currentLexem;
                        }
                        goto gotoDefault;
                    case LexemKind::Assign:
                        if (isSimpleExpr) {
                            goto gotoDefault;
                        }
                        if (GetLexem(1) && GetLexem(1)->kind == LexemKind::Assign) {
                            m_exprLexems.emplace_back(m_lexems[m_currentLexem]);
                            ++m_currentLexem;
                            goto gotoDefault;
                        }
                        SR_FALLTHROUGH;
                    case LexemKind::Comma:
                        if (deep == 0) {
                            break;
                        }
                        SR_FALLTHROUGH;
                    default: {
                    gotoDefault:
                        if (lexemKind == LexemKind::Identifier) {
                            if (!allowIdentifier) {
                                break;
                            }
                        }
                        else {
                            allowIdentifier = true;
                        }

                        m_exprLexems.emplace_back(m_lexems[m_currentLexem]);
                        ++m_currentLexem;

                        goto retry;
                    }
                }

                break;
            }
            case LexemKind::ClosingCurlyBracket:
            case LexemKind::ClosingBracket:
            case LexemKind::ClosingSquareBracket:
                allowIdentifier = false;
                if (deep == 0) {
                    break;
                }
                m_exprLexems.emplace_back(m_lexems[m_currentLexem]);
                ++m_currentLexem;
                --deep;
                goto retry;
        }

        if (deep != 0) {
            if (InBounds()) {
                m_result = SRSLResult(SRSLReturnCode::IncompleteExpression, GetCurrentLexem());
            }
            else {
                m_result = SRSLResult(SRSLReturnCode::IncompleteExpression);
            }
            return;
        }

        if (m_exprLexems.empty()) {
            m_result = SRSLResult(SRSLReturnCode::EmptyExpression, GetCurrentLexem());
            return;
        }

        auto&& [pExpr, result] = SR_SRSL_NS::SRSLMathExpression::Instance().Analyze(m_pAllocator, m_exprLexems);
        m_expr = pExpr;
        m_result = std::move(result);
    }

    void SRSLLexicalAnalyzer::ProcessDecorators() {
        SR_TRACY_ZONE;

        m_decorators = AllocateLexicalUnit<SRSLDecorators>(*m_pAllocator);

    retry:
        if (!InBounds()) {
            m_result = SRSLResult(SRSLReturnCode::InvalidDecorator, nullptr);
            return;
        }

        switch (m_lexems[m_currentLexem].kind) {
            case LexemKind::OpeningSquareBracket: {
                if (!m_states.empty() && m_states.back() == LXAState::Decorators) {
                    m_states.emplace_back(LXAState::Decorator);
                    m_decorators->decorators.emplace_back(SRSLDecorator(*m_pAllocator));
                    ++m_currentLexem;
                    goto retry;
                }
                else {
                    m_states.emplace_back(LXAState::Decorators);
                    ++m_currentLexem;
                    goto retry;
                }
            }
            case LexemKind::Identifier: {
                if (!m_states.empty() && m_states.back() == LXAState::DecoratorArgs) {
                    ProcessExpression();

                    if (IsHasErrors()) {
                        return;
                    }

                    m_decorators->decorators.back().args.emplace_back(SR_UTILS_NS::Exchange(m_expr, nullptr));

                    goto retry;
                }
                else if (!m_states.empty() && m_states.back() == LXAState::Decorator) {
                    m_decorators->decorators.back().name = GetCurrentLexem()->value;
                    ++m_currentLexem;
                    goto retry;
                }
                break;
            }
            case LexemKind::ClosingSquareBracket: {
                if (!m_states.empty() && m_states.back() == LXAState::Decorator) {
                    m_states.pop_back();
                    ++m_currentLexem;
                    goto retry;
                }
                else if (!m_states.empty() && m_states.back() == LXAState::Decorators) {
                    m_states.pop_back();
                    ++m_currentLexem;
                    return;
                }
                break;
            }
            case LexemKind::Comma: {
                if (!m_states.empty() && m_states.back() == LXAState::Decorators) {
                    ++m_currentLexem;
                    goto retry;
                }
                else if (!m_states.empty() && m_states.back() == LXAState::DecoratorArgs) {
                    ++m_currentLexem;

                    ProcessExpression();

                    if (IsHasErrors()) {
                        return;
                    }

                    m_decorators->decorators.back().args.emplace_back(SR_UTILS_NS::Exchange(m_expr, nullptr));

                    goto retry;
                }
                break;
            }
            case LexemKind::OpeningBracket: {
                if (!m_states.empty() && m_states.back() == LXAState::DecoratorArgs) {
                    ++m_currentLexem;

                    ProcessExpression();

                    if (IsHasErrors()) {
                        return;
                    }

                    m_decorators->decorators.back().args.emplace_back(SR_UTILS_NS::Exchange(m_expr, nullptr));

                    goto retry;
                }
                else if (!m_states.empty() && m_states.back() == LXAState::Decorator) {
                    m_states.back() = LXAState::DecoratorArgs;
                    goto retry;
                }
                break;
            }
            case LexemKind::ClosingBracket: {
                if (!m_states.empty() && m_states.back() == LXAState::DecoratorArgs) {
                    m_states.back() = LXAState::Decorator;
                    ++m_currentLexem;
                    goto retry;
                }
                break;
            }
            case LexemKind::Assign:
                m_result = SRSLResult(SRSLReturnCode::InvalidAssign, GetCurrentLexem());
                return;
            case LexemKind::Plus:
            case LexemKind::Minus:
            case LexemKind::Negation:
            case LexemKind::Integer:
                if (!m_states.empty() && m_states.back() == LXAState::DecoratorArgs) {
                    ProcessExpression();
                    if (IsHasErrors()) {
                        return;
                    }

                    m_decorators->decorators.back().args.emplace_back(SR_UTILS_NS::Exchange(m_expr, nullptr));

                    goto retry;
                }
                break;
            default:
                break;
        }

        m_result = SRSLResult(SRSLReturnCode::UnexceptedLexem, GetCurrentLexem());
    }

    bool SRSLLexicalAnalyzer::IsHasErrors() const noexcept {
        return m_result.HasErrors();
    }

    SRSLLexicalUnit* SRSLLexicalAnalyzer::TryProcessIdentifier() {
        SR_TRACY_ZONE;

        auto&& pCurrent = GetCurrentLexem();
        const uint64_t currentLexem = m_currentLexem;

        if (pCurrent->value == "return") {
            ++m_currentLexem;
            if (InBounds() && GetCurrentLexem()->kind != LexemKind::Semicolon) {
                ProcessExpression();
                if (IsHasErrors()) {
                    return nullptr;
                }
            }
            return AllocateLexicalUnit<SRSLReturn>(*m_pAllocator, SR_UTILS_NS::Exchange(m_expr, nullptr));
        }

        if (auto&& pNext = GetLexem(1); pNext && pNext->kind == LexemKind::OpeningSquareBracket) {
            ProcessExpression(true);
        }
        else {
            m_expr = AllocateLexicalUnit<SRSLExpr>(*m_pAllocator, pCurrent->value);
            ++m_currentLexem;
        }

        if (pCurrent = GetCurrentLexem(); pCurrent && pCurrent->kind == LexemKind::Identifier) {
            auto&& pTypeExpr = SR_UTILS_NS::Exchange(m_expr, nullptr);
            ProcessExpression(true);
            auto&& pNameExpr = SR_UTILS_NS::Exchange(m_expr, nullptr);

            if (IsHasErrors()) {
                return nullptr;
            }

            /// переменная имеющая значение: "type[...] name[...] = value;"
            if (pCurrent = GetCurrentLexem(); pCurrent && pCurrent->kind == LexemKind::Assign) {
                auto&& pVariable = AllocateLexicalUnit<SRSLVariable>(*m_pAllocator);

                pVariable->pDecorators = SR_UTILS_NS::Exchange(m_decorators, nullptr);
                pVariable->pType = SR_UTILS_NS::Exchange(pTypeExpr, nullptr);
                pVariable->pName = SR_UTILS_NS::Exchange(pNameExpr, nullptr);

                ++m_currentLexem;

                ProcessExpression();

                pVariable->pExpr = SR_UTILS_NS::Exchange(m_expr, nullptr);

                if (IsHasErrors()) {
                    return nullptr;
                }

                return pVariable;
            }
            /// переменная имеющая значение: "type[...] name[...] = value;"
            else if (pCurrent && pCurrent->kind == LexemKind::OpeningBracket) {
                auto&& pFunction = AllocateLexicalUnit<SRSLFunction>(*m_pAllocator);

                pFunction->pDecorators = SR_UTILS_NS::Exchange(m_decorators, nullptr);
                pFunction->pType = SR_UTILS_NS::Exchange(pTypeExpr, nullptr);
                pFunction->pName = SR_UTILS_NS::Exchange(pNameExpr, nullptr);

                return pFunction;
            }
            /// обычная переменная типа "type[...] name[...];"
            else if (pTypeExpr && pNameExpr) {
                auto&& pVariable = AllocateLexicalUnit<SRSLVariable>(*m_pAllocator);

                pVariable->pType = SR_UTILS_NS::Exchange(pTypeExpr, nullptr);
                pVariable->pName = SR_UTILS_NS::Exchange(pNameExpr, nullptr);
                pVariable->pDecorators = SR_UTILS_NS::Exchange(m_decorators, nullptr);

                return pVariable;
            }

            if (InBounds()) {
                m_result = SRSLResult(SRSLReturnCode::UnexceptedLexem, GetCurrentLexem());
            }
            else {
                m_result = SRSLResult(SRSLReturnCode::UnexceptedLexem);
            }

            return nullptr;
        }

        m_currentLexem = static_cast<int64_t>(currentLexem);

        return nullptr;
    }

    bool SRSLLexicalAnalyzer::ProcessInBuiltName() {
        if (GetCurrentLexem()->value == "else") {
            if (auto&& pNext = GetLexem(1); pNext->value == "if") {
                ++m_currentLexem;
            }
            ++m_currentLexem;
            m_lexicalTree.back()->lexicalTree.emplace_back(AllocateLexicalUnit<SRSLIfStatement>(*m_pAllocator, true));
            m_states.emplace_back(LXAState::IfStatement);
            return true;
        }

        if (GetCurrentLexem()->value == "if") {
            ++m_currentLexem;
            m_lexicalTree.back()->lexicalTree.emplace_back(AllocateLexicalUnit<SRSLIfStatement>(*m_pAllocator));
            m_states.emplace_back(LXAState::IfStatement);
            return true;
        }

        if (GetCurrentLexem()->value == "for") {
            ++m_currentLexem;
            m_lexicalTree.back()->lexicalTree.emplace_back(AllocateLexicalUnit<SRSLForStatement>(*m_pAllocator));
            m_states.emplace_back(LXAState::ForStatement);
            return true;
        }

        if (GetCurrentLexem()->value == "while") {
            ++m_currentLexem;
            m_lexicalTree.back()->lexicalTree.emplace_back(AllocateLexicalUnit<SRSLWhileStatement>(*m_pAllocator));
            m_states.emplace_back(LXAState::WhileStatement);
            return true;
        }

        if (GetCurrentLexem()->value == "struct") {
            ++m_currentLexem;

            auto&& pStructureStatement = AllocateLexicalUnit<SRSLStructureStatement>(*m_pAllocator);
            pStructureStatement->pName = SRSLExpr::CreateStringExpression(*m_pAllocator, GetCurrentLexem()->value);

            ++m_currentLexem;

            m_lexicalTree.back()->lexicalTree.emplace_back(pStructureStatement);
            m_states.emplace_back(LXAState::StructureStatement);
            return true;
        }

        return false;
    }
}
