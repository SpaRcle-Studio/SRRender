//
// Created by Monika on 30.01.2023.
//

#include <Graphics/SRSL/ICodeGenerator.h>

namespace SR_SRSL_NS {
    void ISRSLCodeGenerator::Clear() {
        m_result = SRSLResult();
        m_tabs.reserve(128);
    }

    SR_UTILS_NS::StringView ISRSLCodeGenerator::GenerateTab(int32_t deep) const {
        if (deep <= 0) {
            return SR_UTILS_NS::StringView();
        }

        if (m_tabs.size() < deep * 4) {
            m_tabs.resize(deep * 4, ' ');
        }
        return SR_UTILS_NS::StringView(m_tabs).substr(0, deep * 4);
    }
}
