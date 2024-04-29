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
        using SSBO = int32_t;
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

        SR_NODISCARD VirtualSSBO AllocateSSBO(VirtualSSBO virtualSSBO, uint32_t size, SSBOUsage usage);

        bool FreeSSBO(VirtualSSBO* pSSBO);
        BindResult BindSSBO(VirtualSSBO ssbo) noexcept;

    private:
        bool FreeMemory(SSBO* pSSBO) const;
        bool AllocateMemory(SSBO* pSSBO, uint32_t size, SSBOUsage usage) const;

    private:
        SR_HTYPES_NS::ObjectPool<SSBO, VirtualSSBO> m_ssboPool;
        PipelinePtr m_pipeline;
        void* m_identifier = nullptr;

    };
}

#endif //SR_GRAPHICS_ENGINE_SSBO_MANAGER_H
