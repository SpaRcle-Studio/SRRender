//
// Created by Monika on 09.04.2022.
//

#include <Graphics/Loaders/SRSL.h>

#include <Utils/Common/ToString.h>

namespace SR_SRSL_NS {
    void ShaderMacrosParams::InitHash() {
        SR_TRACY_ZONE;

        m_hash = 0;
        m_initialized = true;

        for (const auto& [key, value] : m_params) {
            m_hash = SR_COMBINE_HASHES(m_hash, SR_HASH_STR(key));
            m_hash = SR_COMBINE_HASHES(m_hash, SR_HASH_STR(value));
        }
    }

    SR_UTILS_NS::SRHashType ShaderMacrosParams::GetHash() const {
        if (m_initialized) {
            return m_hash;
        }
        const_cast<ShaderMacrosParams*>(this)->InitHash();
        return m_hash;
    }

    const ShaderMacrosParams& ShaderMacrosParams::GetDefault() {
        static ShaderMacrosParams params;
        return params;
    }

    std::string ShaderMacrosParams::GetHashStr() const {
        std::string hashStr = SR_UTILS_NS::ToString(GetHash());
        return hashStr;
    }
}
