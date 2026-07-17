//
// Created by Monika on 05.08.2023.
//

#include <Graphics/SRSL/PreProcessor.h>
#include <Graphics/SRSL/Lexer.h>
#include <Graphics/SRSL/MathExpression.h>
#include <Graphics/SRSL/Evaluator.h>

namespace SR_SRSL_NS {
    SRSLPreProcessor::OutResult SRSLPreProcessor::Process(SR_UTILS_NS::Vector<Lexem>&& lexems, Includes& includes, ShaderParams& params) {
        SR_TRACY_ZONE;

        Clear();

        m_lexems = SR_UTILS_NS::Exchange(lexems, { });
        m_includes = std::move(includes);
        m_params = &params;

        while (InBounds() && !IsHasErrors()) {
            ProcessMain();
        }

        includes = std::move(m_includes);

        {
            SR_TRACY_ZONE_N("Remove macro end lexems");
            std::erase_if(m_lexems, [](const Lexem& lexem) {
                return lexem.kind == LexemKind::MacroEnd;
            });
        }

        return std::make_pair(SR_UTILS_NS::Exchange(m_lexems, { }), std::move(m_result));
    }

    void SRSLPreProcessor::Clear() {
        m_currentLexem = 0;
        m_deadBranches = 0;
        m_lexems.clear();
        m_result = SRSLResult();
        m_state = PPState::Idle;
        m_ifStack = {};
        m_ifStack.push(true);
        m_includes.clear();
        m_include.clear();
    }

    const Lexem* SRSLPreProcessor::GetLexem(int64_t offset) const {
        if (m_currentLexem + offset < static_cast<int64_t>(m_lexems.size()) && m_currentLexem + offset >= 0) {
            return &m_lexems.at(m_currentLexem + offset);
        }

        return nullptr;
    }

    bool SRSLPreProcessor::InBounds() const noexcept {
        return m_currentLexem < m_lexems.size();
    }

    bool SRSLPreProcessor::IsHasErrors() const noexcept {
        return m_result.HasErrors();
    }

    const Lexem* SRSLPreProcessor::GetCurrentLexem() const {
        return GetLexem(0);
    }

    void SRSLPreProcessor::ProcessMain() {
        if (m_lexems[m_currentLexem].kind != LexemKind::Macro && m_state == PPState::Idle) {
            if (!m_ifStack.top()) {
                m_lexems.erase(m_lexems.begin() + m_currentLexem);
                return;
            }
        }

        switch (m_lexems[m_currentLexem].kind) {
            case LexemKind::Macro:
                if (m_state != PPState::Idle) {
                    m_result.AddError(SRSLMessage(SRSLReturnCode::UnknownLexem, GetCurrentLexem()).SetDescription("Unexpected macro!"));
                    return;
                }
                m_state = PPState::MacroName;
                m_lexems.erase(m_lexems.begin() + m_currentLexem);
                break;
            case LexemKind::OpeningAngleBracket:
                if (m_state == PPState::IncludeOpen) {
                    m_state = PPState::IncludePath;
                    m_lexems.erase(m_lexems.begin() + m_currentLexem);
                    break;
                }
                ++m_currentLexem;
                break;
            case LexemKind::ClosingAngleBracket:
                if (m_state == PPState::IncludePath) {
                    m_state = PPState::Idle;
                    m_lexems.erase(m_lexems.begin() + m_currentLexem);

                    auto&& includePath = SR_UTILS_NS::ResourceManager::Instance().GetResPath().Concat(m_include);
                    if (!includePath.Exists(SR_UTILS_NS::Path::Type::File)) {
                        m_result.AddError(SRSLMessage(SRSLReturnCode::IncludeNotExists, GetCurrentLexem()))
                            .SetDescription(m_include);
                        return;
                    }

                    SRSLInclude& include = m_includes.emplace_back();
                    include.name = SR_EXCHANGE(m_include, {});

                    auto&& lexems = SR_SRSL_NS::SRSLLexer::Instance().Parse(includePath, include.buffer, m_include.size());
                    if (lexems.empty()) {
                        SR_ERROR("SRSLPreProcessor::ProcessMain() : failed to parse lexems!\n\tPath: " + includePath.ToString());
                        m_result.AddError(SRSLMessage(SRSLReturnCode::IncludeError, GetCurrentLexem())).SetDescription(includePath.ToString());
                        return;
                    }

                    m_lexems.insert(m_lexems.begin() + m_currentLexem, lexems.begin(), lexems.end());

                    break;
                }
                ++m_currentLexem;
                break;
            case LexemKind::Identifier:
                if (m_state == PPState::MacroName) {
                    std::string_view value = GetCurrentLexem()->value;

                    if (m_ifStack.top() && value == "include") {
                        m_state = PPState::IncludeOpen;
                        m_lexems.erase(m_lexems.begin() + m_currentLexem);
                    }
                    else if (value == "define") {
                        m_lexems.erase(m_lexems.begin() + m_currentLexem);
                        std::string_view macroName = GetCurrentLexem() ? GetCurrentLexem()->value : "";
                        m_params->AddDefine(macroName);
                        m_lexems.erase(m_lexems.begin() + m_currentLexem);
                        m_state = PPState::Idle;
                    }
                    else if (value == "undef") {
                        m_lexems.erase(m_lexems.begin() + m_currentLexem);
                        std::string_view macroName = GetCurrentLexem() ? GetCurrentLexem()->value : "";
                        m_params->RemoveDefine(macroName);
                        m_lexems.erase(m_lexems.begin() + m_currentLexem);
                        m_state = PPState::Idle;
                    }
                    else if (value == "if" || value == "ifdef" || value == "ifndef")
                    {
                        if (!m_ifStack.top()) {
                            m_lexems.erase(m_lexems.begin() + m_currentLexem);
                            m_state = PPState::Idle;
                            ++m_deadBranches;
                            break;
                        }
                        m_state = PPState::Idle;
                        m_lexems.erase(m_lexems.begin() + m_currentLexem);
                        if (value == "if") {
                            const int64_t startLexem = m_currentLexem;
                            m_expressionLexems.clear();
                            while (GetCurrentLexem()->kind != LexemKind::MacroEnd) {
                                m_expressionLexems.emplace_back(*GetCurrentLexem());
                                m_currentLexem++;
                            }

                            m_lexems.erase(m_lexems.begin() + startLexem, m_lexems.begin() + m_currentLexem);

                            auto&& result = SRSLMathExpression::Instance().Analyze(std::move(m_expressionLexems));
                            if (!result.first) {
                                SR_ERROR("SRSLPreProcessor::ProcessMain() : failed to evaluate expression!\n\tExpression: {}", result.second.ToString(m_includes));
                                m_ifStack.push(false);
                            }
                            else {
                                const bool expression = SRSLEvaluator::Instance().MacroEvaluate(result.first, *m_params);
                                m_ifStack.push(expression && m_ifStack.top());
                                delete result.first;
                            }
                        }
                        else if (value == "ifdef") {
                            std::string_view macroName = GetCurrentLexem() ? GetCurrentLexem()->value : "";
                            m_lexems.erase(m_lexems.begin() + m_currentLexem);
                            m_ifStack.push(m_params->IsDefined(macroName) && m_ifStack.top());
                        }
                        else if (value == "ifndef") {
                            std::string_view macroName = GetCurrentLexem() ? GetCurrentLexem()->value : "";
                            m_lexems.erase(m_lexems.begin() + m_currentLexem);
                            m_ifStack.push(!m_params->IsDefined(macroName) && m_ifStack.top());
                        }
                        else {
                            SRHalt("Not implemented!");
                        }
                    }
                    else if (value == "else" && m_deadBranches == 0) {
                        m_state = PPState::Idle;
                        if (m_ifStack.size() <= 1) {
                            m_result.AddError(SRSLMessage(SRSLReturnCode::UnknownLexem, GetCurrentLexem()).SetDescription("Unexpected else!"));
                            return;
                        }
                        bool top = m_ifStack.top();
                        m_ifStack.pop();
                        m_ifStack.push(!top && m_ifStack.top());
                        m_lexems.erase(m_lexems.begin() + m_currentLexem);
                    }
                    else if (value == "endif") {
                        if (m_deadBranches > 0) {
                            --m_deadBranches;
                            m_lexems.erase(m_lexems.begin() + m_currentLexem);
                            m_state = PPState::Idle;
                            break;
                        }

                        m_state = PPState::Idle;
                        if (m_ifStack.size() <= 1) {
                            m_result.AddError(SRSLMessage(SRSLReturnCode::UnknownLexem, GetCurrentLexem()).SetDescription("Unexpected endif!"));
                            return;
                        }
                        m_ifStack.pop();
                        m_lexems.erase(m_lexems.begin() + m_currentLexem);
                    }
                    else if (m_ifStack.top()) {
                        m_result.AddError(SRSLMessage(SRSLReturnCode::UnknownLexem, GetCurrentLexem()).SetDescription("Unknown macro!"));
                    }
                    else {
                        m_lexems.erase(m_lexems.begin() + m_currentLexem);
                        m_state = PPState::Idle;
                    }
                    break;
                }
                SR_FALLTHROUGH;
            case LexemKind::Integer:
            case LexemKind::Plus:
            case LexemKind::Minus:
            case LexemKind::OpeningSquareBracket:
            case LexemKind::ClosingSquareBracket:
            case LexemKind::OpeningCurlyBracket:
            case LexemKind::ClosingCurlyBracket:
            case LexemKind::OpeningBracket:
            case LexemKind::ClosingBracket:
            case LexemKind::Exponentiation:
            case LexemKind::Assign:
            case LexemKind::Dot:
            case LexemKind::Comma:
            case LexemKind::Negation:
            case LexemKind::Semicolon:
            case LexemKind::Percent:
            case LexemKind::Divide:
                switch (m_state) {
                    case PPState::Idle: {
                        if (!m_ifStack.top()) {
                            m_lexems.erase(m_lexems.begin() + m_currentLexem);
                            break;
                        }
                        ++m_currentLexem;
                        break;
                    }
                    case PPState::IncludePath:
                        m_include += GetCurrentLexem()->value;
                        m_lexems.erase(m_lexems.begin() + m_currentLexem);
                        break;
                    default:
                        m_result.AddError(SRSLMessage(SRSLReturnCode::UnknownLexem, GetCurrentLexem()));
                        return;
                }
                break;
            default:
                ++m_currentLexem;
                break;
        }
    }
}