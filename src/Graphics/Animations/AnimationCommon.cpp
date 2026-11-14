//
// Created by Monika on 14.07.2024.
//

#include <Graphics/Animations/AnimationCommon.h>

#include <Codegen/AnimationCommon.generated.hpp>

namespace SR_ANIMATIONS_NS {
    IAnimationDataSet::IAnimationDataSet()
        : SR_HTYPES_NS::SharedPtr<IAnimationDataSet>(this, SR_UTILS_NS::SharedPtrPolicy::Automatic)
    { }

    SR_UTILS_NS::Optional<bool> IAnimationDataSet::GetBool(const SR_UTILS_NS::StringAtom& name) const { /** NOLINT */
        if (auto&& pIt = m_boolTable.find(name); pIt != m_boolTable.end()) {
            return pIt->second;
        }

        return m_parent ? m_parent->GetBool(name) : SR_UTILS_NS::Optional<bool>();
    }

    SR_UTILS_NS::Optional<int32_t> IAnimationDataSet::GetInt(const SR_UTILS_NS::StringAtom& name) const { /** NOLINT */
        if (auto&& pIt = m_intTable.find(name); pIt != m_intTable.end()) {
            return pIt->second;
        }

        return m_parent ? m_parent->GetInt(name) : SR_UTILS_NS::Optional<int32_t>();
    }

    SR_UTILS_NS::Optional<float_t> IAnimationDataSet::GetFloat(const SR_UTILS_NS::StringAtom& name) const { /** NOLINT */
        if (auto&& pIt = m_floatTable.find(name); pIt != m_floatTable.end()) {
            return pIt->second;
        }

        return m_parent ? m_parent->GetFloat(name) : SR_UTILS_NS::Optional<float_t>();
    }

    SR_UTILS_NS::Optional<SR_UTILS_NS::String> IAnimationDataSet::GetString(const SR_UTILS_NS::StringAtom& name) const { /** NOLINT */
        if (auto&& pIt = m_stringTable.find(name); pIt != m_stringTable.end()) {
            return pIt->second;
        }

        return m_parent ? m_parent->GetString(name) : SR_UTILS_NS::Optional<SR_UTILS_NS::String>();
    }
}
