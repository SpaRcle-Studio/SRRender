//
// Created by Monika on 09.04.2022.
//

#ifndef SR_ENGINE_SRSL_H
#define SR_ENGINE_SRSL_H

#include <Graphics/Pipeline/ShaderUtils.h>
#include <Graphics/Loaders/ShaderProperties.h>
#include <Graphics/Loaders/SRSLParser.h>
#include <Graphics/SRSL/ShaderType.h>

#include <Utils/Types/SortedVector.h>

namespace SR_SRSL_NS {
    struct ShaderParams : public SR_UTILS_NS::IResourceVariant {
    private:
        struct Entry {
            SR_UTILS_NS::StringAtom key;
            uint32_t position = 0;
            uint32_t size = 0;

            SR_NODISCARD bool operator<(const Entry& other) const {
                return key < other.key;
            }
            SR_NODISCARD bool operator==(const Entry& other) const {
                return key == other.key;
            }

            SR_NODISCARD std::string_view GetValue(const std::string& buffer) const {
                if (position + size > buffer.size() || size == 0) {
                    return {};
                }
                return std::string_view(buffer.data() + position, size);
            }
        };
    public:
        void InitHash();

        void SetFrom(const ShaderParams& other);

        static const ShaderParams& GetDefault();

        SR_NODISCARD SR_UTILS_NS::SRHashType GetHash() const override;
        SR_NODISCARD std::string GetHashStr() const;
        SR_NODISCARD const std::string& GetBuffer() const { return m_buffer; }
        SR_NODISCARD bool empty() const { return m_params.empty(); }
        SR_NODISCARD const SR_HTYPES_NS::SortedVector<Entry>& GetParams() const { return m_params; }
        SR_NODISCARD const SR_UTILS_NS::VertexLayoutDescription& GetVertexLayoutDescription() const { return m_vertexLayoutDescription; }

        void Clear();

        SR_NODISCARD bool IsDefined(std::string_view key) const;

        void SetParam(SR_UTILS_NS::StringAtom key, std::string_view value);
        void AddDefine(SR_UTILS_NS::StringAtom define);
        void RemoveDefine(SR_UTILS_NS::StringAtom define);

        void SetVertexLayoutDescription(const SR_UTILS_NS::VertexLayoutDescription& description) {
            m_vertexLayoutDescription = description;
            m_initialized = false;
        }

        SR_NODISCARD std::string ToString() const {
            std::string result;
            result.reserve(m_buffer.size() + m_params.size() * 16);
            for (const auto& entry : m_params) {
                result += "{}={};"_format(entry.key, entry.GetValue(m_buffer));
            }
            return result;
        }

    private:
        SR_HTYPES_NS::SortedVector<Entry> m_params;
        std::string m_buffer;

        SR_UTILS_NS::SRHashType m_hash = 0;
        bool m_initialized = false;
        SR_UTILS_NS::VertexLayoutDescription m_vertexLayoutDescription;

    };
}

#endif //SR_ENGINE_SRSL_H
