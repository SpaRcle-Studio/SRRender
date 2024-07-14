//
// Created by Monika on 14.07.2024.
//

#include <Graphics/Animations/AnimationCommon.h>

namespace SR_ANIMATIONS_NS {
    std::optional<bool> IAnimationDataSet::GetBool(const SR_UTILS_NS::StringAtom& name) const { /** NOLINT */
        if (auto&& pIt = m_boolTable.find(name); pIt != m_boolTable.end()) {
            return pIt->second;
        }

        return m_parent ? m_parent->GetBool(name) : std::nullopt;
    }

    std::optional<int32_t> IAnimationDataSet::GetInt(const SR_UTILS_NS::StringAtom& name) const { /** NOLINT */
        if (auto&& pIt = m_intTable.find(name); pIt != m_intTable.end()) {
            return pIt->second;
        }

        return m_parent ? m_parent->GetInt(name) : std::nullopt;
    }

    std::optional<float_t> IAnimationDataSet::GetFloat(const SR_UTILS_NS::StringAtom& name) const { /** NOLINT */
        if (auto&& pIt = m_floatTable.find(name); pIt != m_floatTable.end()) {
            return pIt->second;
        }

        return m_parent ? m_parent->GetFloat(name) : std::nullopt;
    }

    std::optional<std::string> IAnimationDataSet::GetString(const SR_UTILS_NS::StringAtom& name) const { /** NOLINT */
        if (auto&& pIt = m_stringTable.find(name); pIt != m_stringTable.end()) {
            return pIt->second;
        }

        return m_parent ? m_parent->GetString(name) : std::nullopt;
    }
}
