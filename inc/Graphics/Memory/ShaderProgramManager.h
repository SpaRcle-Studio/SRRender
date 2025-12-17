//
// Created by Monika on 11.07.2022.
//

#ifndef SR_ENGINE_SHADERPROGRAMMANAGER_H
#define SR_ENGINE_SHADERPROGRAMMANAGER_H

#include <Graphics/Pipeline/IShaderProgram.h>

#include <Utils/Common/Singleton.h>
#include <Utils/Types/Map.h>
#include <Utils/Types/ObjectPool.h>

namespace SR_GRAPH_NS {
    class Pipeline;
}

namespace SR_GRAPH_NS::Memory {
    struct SR_GRAPHICS_DLL_API VirtualProgramInfo : public SR_UTILS_NS::NonCopyable {
    public:
        using Identifier = uint64_t;
        using ShaderProgram = int32_t;
        struct ShaderProgramInfo {
            ShaderProgram id = SR_ID_INVALID;
            bool depth = false;
            uint8_t samples = 1;

            SR_NODISCARD SR_FORCE_INLINE bool Valid() const {
                return id != SR_ID_INVALID;
            }
        };
        using DataType = std::pair<Identifier, ShaderProgramInfo>;
        static constexpr uint32_t MAX_DATA_COUNT = 128;

    public:
        VirtualProgramInfo() = default;
        ~VirtualProgramInfo() override = default;

        VirtualProgramInfo(VirtualProgramInfo&& ref) noexcept {
            memmove(m_data, ref.m_data, sizeof(DataType) * MAX_DATA_COUNT);
            m_dataUsed = SR_UTILS_NS::Exchange(ref.m_dataUsed, {});
            m_createInfo = SR_UTILS_NS::Exchange(ref.m_createInfo, {});
        }

        VirtualProgramInfo& operator=(VirtualProgramInfo&& ref) noexcept {
            memmove(m_data, ref.m_data, sizeof(DataType) * MAX_DATA_COUNT);
            m_dataUsed = SR_UTILS_NS::Exchange(ref.m_dataUsed, {});
            m_createInfo = SR_UTILS_NS::Exchange(ref.m_createInfo, {});
            return *this;
        }

        SR_NODISCARD bool Valid() const { return m_createInfo.Validate(); }

        SR_FORCE_INLINE ShaderProgramInfo* SetProgramInfo(Identifier identifier, const ShaderProgramInfo& info) {
            for (uint32_t i = 0; i < m_dataUsed; ++i) {
                if (m_data[i].first == identifier) SR_LIKELY_ATTRIBUTE {
                    m_data[i].second = info;
                    return &m_data[i].second;
                }
            }

            if (m_dataUsed < MAX_DATA_COUNT) {
                m_data[m_dataUsed] = std::make_pair(identifier, info);
                ++m_dataUsed;
                return &m_data[m_dataUsed - 1].second;
            }

            SRHalt("Exceeded maximum shader program info count!");
            return nullptr;
        }

        SR_NODISCARD SR_FORCE_INLINE bool HasProgram(Identifier identifier) const {
            const auto* pBegin = m_data;
            const auto* pEnd = pBegin + MAX_DATA_COUNT;

            while (pBegin != pEnd) {
                if (pBegin->first == identifier) SR_LIKELY_ATTRIBUTE {
                    return true;
                }
                ++pBegin;
            }
            return false;
        }

        SR_NODISCARD SR_FORCE_INLINE ShaderProgramInfo* GetProgramInfo(Identifier identifier) {
            for (uint32_t i = 0; i < m_dataUsed; ++i) {
                if (m_data[i].first == identifier) SR_LIKELY_ATTRIBUTE {
                    return &m_data[i].second;
                }
            }

            return nullptr;
        }

        SR_NODISCARD SR_FORCE_INLINE const ShaderProgramInfo* GetProgramInfo(Identifier identifier) const noexcept {
            for (uint32_t i = 0; i < m_dataUsed; ++i) {
                if (m_data[i].first == identifier) SR_LIKELY_ATTRIBUTE {
                    return &m_data[i].second;
                }
            }

            return nullptr;
        }

        SR_NODISCARD SR_FORCE_INLINE int32_t GetProgramId(Identifier identifier) const noexcept {
            for (uint32_t i = 0; i < m_dataUsed; ++i) {
                if (m_data[i].first == identifier) SR_LIKELY_ATTRIBUTE {
                    return m_data[i].second.id;
                }
            }

            return SR_ID_INVALID;
        }

        SR_FORCE_INLINE void ResetData() noexcept {
            for (uint32_t i = 0; i < m_dataUsed; ++i) {
                m_data[i] = std::make_pair(0, ShaderProgramInfo());
            }
            m_dataUsed = 0;
        }

        SR_FORCE_INLINE DataType* DataBegin() noexcept { return m_data; }
        SR_FORCE_INLINE DataType* DataEnd() noexcept { return m_data + m_dataUsed; }

        SR_FORCE_INLINE DataType* EraseData(DataType* pData) noexcept {
            if (pData < m_data || pData >= m_data + m_dataUsed) {
                return pData;
            }

            const auto index = static_cast<uint32_t>(pData - m_data);
            memmove(&m_data[index], &m_data[index + 1], sizeof(DataType) * (m_dataUsed - index - 1));
            --m_dataUsed;
            m_data[m_dataUsed] = std::make_pair(0, ShaderProgramInfo());
            return &m_data[index];
        }

        uint32_t m_dataUsed = 0;
        DataType m_data[MAX_DATA_COUNT] = {};
        SRShaderCreateInfo m_createInfo;

    };

    /**
     * Класс реализует возможность рендера одного объекта в несколько кадровых буферов
    */
    class SR_GRAPHICS_DLL_API ShaderProgramManager : public SR_UTILS_NS::Singleton<ShaderProgramManager> {
        SR_REGISTER_SINGLETON(ShaderProgramManager)
    public:
        using PipelinePtr = SR_HTYPES_NS::SharedPtr<Pipeline>;
        using VirtualProgram = int32_t;
        using ShaderProgram = int32_t;
    private:
        ShaderProgramManager();
        ~ShaderProgramManager() override = default;

    public:
        void SetPipeline(PipelinePtr pPipeline) { m_pipeline = std::move(pPipeline); }

        SR_NODISCARD VirtualProgram ReAllocate(VirtualProgram program, const SRShaderCreateInfo& createInfo);
        SR_NODISCARD VirtualProgram Allocate(const SRShaderCreateInfo& createInfo);

        bool FreeProgram(VirtualProgram* program);
        bool FreeProgram(VirtualProgram program);

        void CollectUnused();

        ShaderBindResult BindProgram(VirtualProgram virtualProgram) noexcept;

        SR_NODISCARD const VirtualProgramInfo* GetInfo(VirtualProgram virtualProgram) const noexcept;
        SR_NODISCARD ShaderProgram GetProgram(VirtualProgram virtualProgram) const noexcept;
        SR_NODISCARD bool IsAvailable(VirtualProgram virtualProgram) const noexcept;
        SR_NODISCARD bool HasProgram(VirtualProgram virtualProgram) const noexcept;

    private:
        SR_NODISCARD VirtualProgramInfo::Identifier GetCurrentIdentifier() const;
        SR_NODISCARD VirtualProgramInfo::ShaderProgramInfo AllocateShaderProgram(const SRShaderCreateInfo& createInfo) const;
        SR_NODISCARD ShaderBindResult BindShaderProgram(VirtualProgramInfo::ShaderProgramInfo& shaderProgramInfo, const SRShaderCreateInfo& createInfo);

    protected:
        void OnSingletonDestroy() override;

    private:
        SR_HTYPES_NS::ObjectPool<VirtualProgramInfo, VirtualProgram> m_programPool;
        PipelinePtr m_pipeline;
        std::vector<void*> m_handles;

    };
}

#endif //SR_ENGINE_SHADERPROGRAMMANAGER_H
