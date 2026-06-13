//
// Created by Monika on 22.01.2023.
//

#ifndef SR_ENGINE_SRSL_LEXER_H
#define SR_ENGINE_SRSL_LEXER_H

#include <Graphics/SRSL/LexerUtils.h>

namespace SR_SRSL_NS {
    class SRSLLexer : public SR_UTILS_NS::Singleton<SRSLLexer> {
        SR_REGISTER_SINGLETON(SRSLLexer)
        using Lexems = std::vector<Lexem>;
        using ProcessedLexem = std::optional<Lexem>;
        using SourceCode = std::vector<std::string>;
    protected:
        ~SRSLLexer() override;

    public:
        SR_NODISCARD Lexems Parse(const SR_UTILS_NS::Path& path, SR_UTILS_NS::String& buffer, uint16_t fileIndex);
        SR_NODISCARD Lexems ParseString(std::string_view code, uint16_t fileIndex);

    private:
        SR_NODISCARD bool InBounds() const noexcept;
        SR_NODISCARD ProcessedLexem ProcessLexem();
        SR_NODISCARD std::string_view ProcessIdentifier();

        SR_NODISCARD Lexems ParseInternal(std::string_view code, uint16_t fileIndex);

        void Clear();

        void SkipSpaces();
        void SkipComment();

    private:
        bool m_macroLine = false;
        std::string_view m_source;
        uint64_t m_offset = 0;
        uint16_t m_fileIndex = 0;
        uint64_t m_line = 0;
        uint64_t m_position = 0;

        Lexems m_lexems;

    };
}

#endif //SR_ENGINE_SRSL_LEXER_H
