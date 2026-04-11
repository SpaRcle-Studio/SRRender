//
// Created by Monika on 09.04.2022.
//

#include <Graphics/Loaders/SRSL.h>

#include <Utils/Common/ToString.h>

namespace SR_SRSL_NS {
    void ShaderParams::InitHash() {
        SR_TRACY_ZONE;

        m_hash = m_vertexLayoutDescription.GetHash();
        m_initialized = true;

        for (const auto& entry : m_params) {
            m_hash = SR_COMBINE_HASHES(m_hash, entry.key.GetHash());
            m_hash = SR_COMBINE_HASHES(m_hash, SR_HASH_STR_VIEW(entry.GetValue(m_buffer)));
        }
    }

    SR_UTILS_NS::SRHashType ShaderParams::GetHash() const {
        if (m_initialized) {
            return m_hash;
        }
        const_cast<ShaderParams*>(this)->InitHash();
        return m_hash;
    }

    const ShaderParams& ShaderParams::GetDefault() {
        static ShaderParams params;
        return params;
    }

    std::string ShaderParams::GetHashStr() const {
        std::string hashStr = SR_UTILS_NS::ToString(GetHash());
        return hashStr;
    }

    void ShaderParams::SetParam(SR_UTILS_NS::StringAtom key, std::string_view value) {
        SR_TRACY_ZONE;
        Entry entry { key, static_cast<uint32_t>(m_buffer.size()), static_cast<uint32_t>(value.size()) };

        if (auto&& pEntry = m_params.Find(entry)) {
            if (pEntry->GetValue(m_buffer) == value) {
                return;
            }
            if (pEntry->size > 0) {
                m_buffer.erase(pEntry->position, pEntry->size);
                for (auto& e: m_params) {
                    if (e.position > pEntry->position) {
                        e.position -= pEntry->size;
                    }
                }
            }
            *pEntry = entry;
        }
        else {
            m_params.Add(entry);
        }
        m_buffer += value;
        m_initialized = false;
    }

    void ShaderParams::AddDefine(SR_UTILS_NS::StringAtom define) {
        SR_TRACY_ZONE;
        Entry entry { define, 0, 0 };

        if (auto&& pEntry = m_params.Find(entry)) {
            if (pEntry->size > 0) {
                m_buffer.erase(pEntry->position, pEntry->size);
                for (auto& e: m_params) {
                    if (e.position > pEntry->position) {
                        e.position -= pEntry->size;
                    }
                }
                m_initialized = false;
            }
        }
        else {
            m_params.Add(entry);
            m_initialized = false;
        }
    }

    void ShaderParams::Clear() {
        m_params.clear();
        m_buffer.clear();
        m_vertexLayoutDescription.Reset();
        m_hash = 0;
        m_initialized = false;
    }

    bool ShaderParams::IsDefined(std::string_view key) const {
        for (const auto& entry : m_params) {
            if (entry.key == key) {
                return true;
            }
        }
        return false;
    }

    void ShaderParams::RemoveDefine(SR_UTILS_NS::StringAtom define) {
        SR_TRACY_ZONE;
        Entry entry { define, 0, 0 };

        if (auto&& pEntry = m_params.Find(entry)) {
            if (pEntry->size > 0) {
                m_buffer.erase(pEntry->position, pEntry->size);
                for (auto& e: m_params) {
                    if (e.position > pEntry->position) {
                        e.position -= pEntry->size;
                    }
                }
            }
            m_params.Remove(*pEntry);
            m_initialized = false;
        }
    }

    void ShaderParams::SetFrom(const ShaderParams& other) {
        SR_TRACY_ZONE;
        m_vertexLayoutDescription = other.m_vertexLayoutDescription;
        m_buffer = other.m_buffer;
        m_params = other.m_params;
        m_hash = other.m_hash;
        m_initialized = other.m_initialized;
    }
}
