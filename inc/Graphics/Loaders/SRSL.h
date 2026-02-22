//
// Created by Monika on 09.04.2022.
//

#ifndef SR_ENGINE_SRSL_H
#define SR_ENGINE_SRSL_H

#include <Graphics/Pipeline/IShaderProgram.h>
#include <Graphics/Loaders/ShaderProperties.h>
#include <Graphics/Loaders/SRSLParser.h>
#include <Graphics/SRSL/ShaderType.h>

namespace SR_SRSL_NS {
    struct ShaderMacrosParams : public SR_UTILS_NS::IResourceVariant {
        void InitHash();

        static const ShaderMacrosParams& GetDefault();

        SR_NODISCARD SR_UTILS_NS::SRHashType GetHash() const override;
        SR_NODISCARD std::string GetHashStr() const;
        SR_NODISCARD bool empty() const { return m_params.empty(); }
        SR_NODISCARD const std::map<SR_UTILS_NS::StringAtom, std::string>& GetParams() const { return m_params; }

        void Clear() {
            m_params.clear();
            m_hash = 0;
            m_initialized = false;
        }

        SR_NODISCARD bool IsDefined(const std::string_view& key) const {
            for (const auto& [k, v] : m_params) {
                if (k == key) {
                    return true;
                }
            }
            return false;
        }

        void SetParam(SR_UTILS_NS::StringAtom key, const std::string& value) {
            m_params[key] = value;
            m_initialized = false;
        }

        void AddDefine(SR_UTILS_NS::StringAtom define) {
            m_params[define];
            m_initialized = false;
        }

        SR_NODISCARD std::string ToString() const {
            std::string result;
            for (const auto& [key, value] : m_params) {
                result += key.ToString() + "=" + value + ";";
            }
            return result;
        }

    private:
        std::map<SR_UTILS_NS::StringAtom, std::string> m_params;
        SR_UTILS_NS::SRHashType m_hash = 0;
        bool m_initialized = false;

    };
}

#endif //SR_ENGINE_SRSL_H
