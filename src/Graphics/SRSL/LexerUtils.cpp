//
// Created by Monika on 17.07.2026.
//

#include <Graphics/SRSL/LexerUtils.h>

namespace SR_SRSL_NS {
    bool IsIdentifier(SR_UTILS_NS::StringView token) noexcept {
        bool isFirst = true;
        for (auto&& tokenChar : token) {
            if (isFirst && SR_MATH_NS::IsNumber(std::string_view(&tokenChar, 1))) {
                return false;
            }
            isFirst = false;

            if ((tokenChar >= 'a' && tokenChar <= 'z') ||
                (tokenChar >= 'A' && tokenChar <= 'Z') ||
                (tokenChar >= '0' && tokenChar <= '9') ||
                (tokenChar == '_'))
            {
                continue;
            }

            return false;
        }

        return true;
    }

    bool IsOperator(SR_UTILS_NS::StringView operation) noexcept {
        constexpr char operators[15] = {
            '+', '-', '!', '.', '~', '>', '^', '<', ':', '?', '|', '&', '%',
        };
        if (operation.size() != 1) {
            return false;
        }
        const char firstChar = operation[0];
        for (const char op : operators) {
            if (firstChar == op) {
                return true;
            }
        }
        return false;
    }
}
