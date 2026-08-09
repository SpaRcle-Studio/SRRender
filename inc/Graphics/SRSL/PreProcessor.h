//
// Created by Monika on 22.01.2023.
//

#ifndef SR_ENGINE_SRSL_PREPROCESSOR_H
#define SR_ENGINE_SRSL_PREPROCESSOR_H

#include <Graphics/SRSL/LexicalTree.h>

#include <Utils/Common/Singleton.h>
#include <Utils/Lexer/LexerUtils.h>

namespace SR_SRSL_NS {
    class SRSLPreProcessor : public SR_UTILS_NS::Singleton<SRSLPreProcessor> {
        SR_REGISTER_SINGLETON(SRSLPreProcessor)
        enum class PPState : uint8_t {
            Idle, Macro, MacroName, IncludeOpen, IncludePath
        };
    public:
        using Includes = SR_UTILS_NS::Vector<SR_UTILS_NS::LexerDetails::LexerInclude>;
        using OutResult = std::pair<SR_UTILS_NS::Vector<SR_UTILS_NS::LexerDetails::Lexem>, SR_UTILS_NS::LexerDetails::LexerResult>;

    public:
        SR_NODISCARD OutResult Process(SR_UTILS_NS::IAllocator* pAllocator, SR_UTILS_NS::Vector<SR_UTILS_NS::LexerDetails::Lexem>&& lexems, Includes& includes, ShaderParams& params);

    private:
        void Clear();

        void ProcessMain();

        SR_NODISCARD bool InBounds() const noexcept;
        SR_NODISCARD bool IsHasErrors() const noexcept;
        SR_NODISCARD const SR_UTILS_NS::LexerDetails::Lexem* GetLexem(int64_t offset) const;
        SR_NODISCARD const SR_UTILS_NS::LexerDetails::Lexem* GetCurrentLexem() const;

    private:
        SR_UTILS_NS::LexerDetails::LexerResult m_result;
        ShaderParams* m_params = nullptr;

        SR_UTILS_NS::IAllocator* m_pAllocator = nullptr;
        SR_UTILS_NS::Vector<SR_UTILS_NS::LexerDetails::Lexem> m_expressionLexems;
        SR_UTILS_NS::Vector<SR_UTILS_NS::LexerDetails::Lexem> m_lexems;
        int64_t m_currentLexem = 0;

        std::string m_include;
        Includes m_includes;

        std::stack<bool> m_ifStack;
        int m_deadBranches = 0;
        PPState m_state = PPState::Idle;

    };
}

#endif //SR_ENGINE_SRSL_PREPROCESSOR_H
