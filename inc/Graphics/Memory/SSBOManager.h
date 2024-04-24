//
// Created by Monika on 24.04.2024.
//

#ifndef SR_GRAPHICS_ENGINE_SSBO_MANAGER_H
#define SR_GRAPHICS_ENGINE_SSBO_MANAGER_H

#include <Utils/Common/Singleton.h>
#include <Utils/Types/ObjectPool.h>

namespace SR_GTYPES_NS {
    class Shader;
}

namespace SR_GRAPH_NS {
    class Pipeline;

    class SSBOManager : public SR_UTILS_NS::Singleton<SSBOManager> {
        SR_REGISTER_SINGLETON(SSBOManager)
        using Super = SR_UTILS_NS::Singleton<SSBOManager>;
        using VirtualSSBO = int32_t;
        struct VirtualSSBOInfo : public SR_UTILS_NS::NonCopyable {
            VirtualSSBOInfo() = default;
            VirtualSSBOInfo(VirtualSSBOInfo&& other) noexcept
                : descriptors(std::move(other.descriptors))
                , SSBO(other.SSBO)
            { }

            VirtualSSBOInfo& operator=(VirtualSSBOInfo&& other) noexcept {
                descriptors = std::move(other.descriptors);
                SSBO = other.SSBO;
                return *this;
            }

            /// first - raw shader handle, second - descriptor index
            std::vector<std::pair<void*, int32_t>> descriptors;
            int32_t SSBO = SR_ID_INVALID;
        };
        using PipelinePtr = SR_HTYPES_NS::SharedPtr<Pipeline>;
    public:
        enum class BindResult : uint8_t {
            None,
            Success,
            Duplicated,
            Failed
        };
    public:
        void SetPipeline(PipelinePtr pPipeline) { m_pipeline = std::move(pPipeline); }

        SR_NODISCARD VirtualSSBO AllocateSSBO(uint32_t size, SSBOUsage usage);
        SR_NODISCARD VirtualSSBO ReAllocateSSBO(VirtualSSBO virtualSSBO, uint32_t size, SSBOUsage usage);

        bool FreeSSBO(VirtualSSBO* pSSBO);
        BindResult BindSSBO(VirtualSSBO ssbo) noexcept;

    private:
        SR_NODISCARD std::pair<void*, int32_t> GetCurrentShaderHandle() const;

        bool FreeMemory(VirtualSSBOInfo& memory) const;
        bool AllocateMemory(VirtualSSBOInfo& memory, uint32_t size, SSBOUsage usage, bool allocSSBO) const;

    private:
        SR_HTYPES_NS::ObjectPool<VirtualSSBOInfo, VirtualSSBO> m_ssboPool;
        PipelinePtr m_pipeline;
        void* m_identifier = nullptr;

    };
}

#endif //SR_GRAPHICS_ENGINE_SSBO_MANAGER_H
