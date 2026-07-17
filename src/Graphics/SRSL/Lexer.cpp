//
// Created by Monika on 22.01.2023.
//

#include <Graphics/SRSL/Lexer.h>
#include <Utils/FileSystem/FileSystem.h>

namespace SR_SRSL_NS {
    SRSLLexer::~SRSLLexer() {
        Clear();
    }

    SRSLLexer::Lexems SRSLLexer::Parse(const SR_UTILS_NS::Path& path, SR_UTILS_NS::String& buffer, uint16_t fileIndex) {
        SR_TRACY_ZONE;

        if (!SR_UTILS_NS::FileSystem::ReadFile(path, buffer) || buffer.empty()) {
            SR_ERROR("SRSLLexer::Parse() : failed to read file!\n\tPath: {}", path);
            return { };
        }

        return ParseInternal(buffer, fileIndex);
    }

    SRSLLexer::Lexems SRSLLexer::ParseString(std::string_view code, uint16_t fileIndex) {
        return ParseInternal(code, fileIndex);
    }

    bool SRSLLexer::InBounds() const noexcept {
        return m_offset < m_source.size();
    }

    SRSLLexer::Lexems SRSLLexer::ParseInternal(std::string_view code, uint16_t fileIndex) {
        SR_TRACY_ZONE;

        Clear();

        m_lexems.reserve(512);
        m_source = code;
        m_fileIndex = fileIndex;

        while (InBounds()) {
            SkipSpaces();

            if (!InBounds()) {
                break;
            }

            if (auto&& lexem = ProcessLexem()) {
                m_lexems.emplace_back(lexem.value());
            }
        }

        return SR_UTILS_NS::Exchange(m_lexems, { });
    }

    SRSLLexer::ProcessedLexem SRSLLexer::ProcessLexem() {
        const char actualChar = m_source[m_offset];

        switch (actualChar) {
            case '[': return Lexem(m_offset++, 1, LexemKind::OpeningSquareBracket, "[", m_fileIndex, m_line, m_position++);
            case ']': return Lexem(m_offset++, 1, LexemKind::ClosingSquareBracket, "]", m_fileIndex, m_line, m_position++);

            case '<': return Lexem(m_offset++, 1, LexemKind::OpeningAngleBracket, "<", m_fileIndex, m_line, m_position++);
            case '>': return Lexem(m_offset++, 1, LexemKind::ClosingAngleBracket, ">", m_fileIndex, m_line, m_position++);

            case '{': return Lexem(m_offset++, 1, LexemKind::OpeningCurlyBracket, "{", m_fileIndex, m_line, m_position++);
            case '}': return Lexem(m_offset++, 1, LexemKind::ClosingCurlyBracket, "}", m_fileIndex, m_line, m_position++);

            case '(': return Lexem(m_offset++, 1, LexemKind::OpeningBracket, "(", m_fileIndex, m_line, m_position++);
            case ')': return Lexem(m_offset++, 1, LexemKind::ClosingBracket, ")", m_fileIndex, m_line, m_position++);

            case '+': return Lexem(m_offset++, 1, LexemKind::Plus, "+", m_fileIndex, m_line, m_position++);
            case '-': return Lexem(m_offset++, 1, LexemKind::Minus, "-", m_fileIndex, m_line, m_position++);
            case '*': return Lexem(m_offset++, 1, LexemKind::Multiply, "*", m_fileIndex, m_line, m_position++);
            case '%': return Lexem(m_offset++, 1, LexemKind::Percent, "%", m_fileIndex, m_line, m_position++);
            case '^': return Lexem(m_offset++, 1, LexemKind::Exponentiation, "^", m_fileIndex, m_line, m_position++);
            case '?': return Lexem(m_offset++, 1, LexemKind::Question, "?", m_fileIndex, m_line, m_position++);
            case ':': return Lexem(m_offset++, 1, LexemKind::Colon, ":", m_fileIndex, m_line, m_position++);
            case '~': return Lexem(m_offset++, 1, LexemKind::Tilda, "~", m_fileIndex, m_line, m_position++);

            case '/': {
                if (m_offset + 1 < m_source.size()) {
                    switch (m_source[m_offset + 1]) {
                        case '/':
                        case '*':
                            SkipComment();
                            return {};
                        default:
                            break;
                    }
                }

                return Lexem(m_offset++, 1, LexemKind::Divide, "/", m_fileIndex, m_line, m_position++);
            }
            case '=': return Lexem(m_offset++, 1, LexemKind::Assign, "=", m_fileIndex, m_line, m_position++);
            case ';': return Lexem(m_offset++, 1, LexemKind::Semicolon, ";", m_fileIndex, m_line, m_position++);
            case '.': return Lexem(m_offset++, 1, LexemKind::Dot, ".", m_fileIndex, m_line, m_position++);
            case ',': return Lexem(m_offset++, 1, LexemKind::Comma, ",", m_fileIndex, m_line, m_position++);
            case '!': return Lexem(m_offset++, 1, LexemKind::Negation, "!", m_fileIndex, m_line, m_position++);
            case '&': return Lexem(m_offset++, 1, LexemKind::And, "&", m_fileIndex, m_line, m_position++);
            case '|': return Lexem(m_offset++, 1, LexemKind::Or, "|", m_fileIndex, m_line, m_position++);

            case '0': return Lexem(m_offset++, 1, LexemKind::Integer, "0", m_fileIndex, m_line, m_position++);
            case '1': return Lexem(m_offset++, 1, LexemKind::Integer, "1", m_fileIndex, m_line, m_position++);
            case '2': return Lexem(m_offset++, 1, LexemKind::Integer, "2", m_fileIndex, m_line, m_position++);
            case '3': return Lexem(m_offset++, 1, LexemKind::Integer, "3", m_fileIndex, m_line, m_position++);
            case '4': return Lexem(m_offset++, 1, LexemKind::Integer, "4", m_fileIndex, m_line, m_position++);
            case '5': return Lexem(m_offset++, 1, LexemKind::Integer, "5", m_fileIndex, m_line, m_position++);
            case '6': return Lexem(m_offset++, 1, LexemKind::Integer, "6", m_fileIndex, m_line, m_position++);
            case '7': return Lexem(m_offset++, 1, LexemKind::Integer, "7", m_fileIndex, m_line, m_position++);
            case '8': return Lexem(m_offset++, 1, LexemKind::Integer, "8", m_fileIndex, m_line, m_position++);
            case '9': return Lexem(m_offset++, 1, LexemKind::Integer, "9", m_fileIndex, m_line, m_position++);

            case '#':
                m_macroLine = true;
                return Lexem(m_offset++, 1, LexemKind::Macro, "#", m_fileIndex, m_line, m_position++);

            case '"': return Lexem(m_offset++, 1, LexemKind::String, "\"", m_fileIndex, m_line, m_position++);

            default:
                break;
        }

        //if (actualChar >= '0' && actualChar <= '9') {
        //    return Lexem(m_offset++, 1, LexemKind::Integer, "0", m_fileIndex, m_line, m_position++);
        //}

        const uint64_t offset = m_offset;
        const uint64_t position = m_position;

        auto&& identifier = ProcessIdentifier();

        const uint64_t length = identifier.size();

        return Lexem(offset, length, LexemKind::Identifier, identifier, m_fileIndex, m_line, position);
    }

    std::string_view SRSLLexer::ProcessIdentifier() {
        uint32_t identifierStart = SR_UINT32_MAX;
        uint32_t identifierLength = 0;

    retry:
        if (InBounds()) {
            const char firstChar = m_source[m_offset];
            if ((firstChar >= 'a' && firstChar <= 'z') || (firstChar >= 'A' && firstChar <= 'Z') || firstChar == '_' || (firstChar >= '0' && firstChar <= '9')) {
                if (identifierStart == SR_UINT32_MAX) {
                    identifierStart = m_offset;
                }
                ++identifierLength;
                ++m_offset;
                ++m_position;
                goto retry;
            }
        }

        if (identifierLength == 0) {
            SR_ERROR("SRSLLexer::ProcessIdentifier() : invalid identifier!"
                 " \n\tPosition: {}"
                 " \n\tIdentifier: \"{}\"", m_offset, m_source[m_offset]
            );
            ++m_offset;
            ++m_position;
        }

        if (identifierStart == SR_UINT32_MAX) {
            return {};
        }
        return std::string_view(m_source.data() + identifierStart, identifierLength);
    }

    void SRSLLexer::SkipSpaces() {
    retry:
        while (InBounds()) {
            for (auto&& spaceChar : SRSL_SPACE_CHARS) {
                if (spaceChar == m_source[m_offset]) {
                    ++m_offset;
                    ++m_position;

                    if (spaceChar == '\n') {
                        if (m_macroLine) {
                            m_macroLine = false;
                            m_lexems.emplace_back(Lexem(m_offset, 1, LexemKind::MacroEnd, "\n", m_fileIndex, m_line, m_position));
                        }
                        ++m_line;
                        m_position = 0;
                    }

                    goto retry;
                }
            }

            break;
        }
    }

    void SRSLLexer::SkipComment() {
        m_offset += 1; /// skip '/'
        m_position += 1;

        switch (m_source[m_offset]) {
            case '/': {
                m_offset += 1; /// skip '/'
                m_position += 1;
                while (m_offset < m_source.size() && m_source[m_offset] != '\n' && m_source[m_offset] != '\r') {
                    m_offset += 1;
                    m_position += 1;
                }
                break;
            }
            case '*': {
                m_offset += 1; /// skip '*'
                m_position += 1;
                while (m_offset + 1 < m_source.size() && !(m_source[m_offset] == '*' && m_source[m_offset + 1] == '/')) {
                    m_offset += 1;
                    m_position += 1;
                }
                /// skip "*/"
                if (m_offset + 1 < m_source.size() && (m_source[m_offset] == '*' && m_source[m_offset + 1] == '/')) {
                    m_offset += 2;
                    m_position += 2;
                }
                break;
            }
            default:
                SRHalt("SRSLLexer::SkipComment() : something went wrong!");
                return;
        }
    }

    void SRSLLexer::Clear() {
        SR_TRACY_ZONE;

        m_macroLine = false;
        m_source = { };
        m_fileIndex = 0;
        m_offset = 0;
        m_lexems.clear();
        m_line = 0;
        m_position = 0;
    }
}