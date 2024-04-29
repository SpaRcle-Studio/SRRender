//
// Created by Monika on 27.04.2024.
//

#ifndef SR_ENGINE_GRAPHICS_DESCRIPTOR_MANAGER_H
#define SR_ENGINE_GRAPHICS_DESCRIPTOR_MANAGER_H

#include <Utils/Common/Singleton.h>
#include <Utils/Types/ObjectPool.h>
#include <Utils/Types/SharedPtr.h>

#include <Graphics/Types/Descriptors.h>

namespace SR_GTYPES_NS {
    class Shader;
}

namespace SR_GRAPH_NS {
    class Pipeline;

    class DescriptorManager : public SR_UTILS_NS::Singleton<DescriptorManager> {
        SR_REGISTER_SINGLETON(DescriptorManager)
        using DescriptorSet = int32_t;
        struct DescriptorSetInfo {
            void* pShaderHandle = nullptr;
            DescriptorSet descriptorSet = SR_ID_INVALID;
        };
        using VirtualDescriptorSet = int32_t;
    public:
        enum class BindResult : uint8_t {
            None,
            Success,
            Duplicated,
            Failed
        };
    public:
        SR_NODISCARD VirtualDescriptorSet AllocateDescriptorSet(VirtualDescriptorSet reallocation = SR_ID_INVALID);
        BindResult Bind(VirtualDescriptorSet virtualDescriptorSet);

        void FreeDescriptorSet(VirtualDescriptorSet* pVirtualDescriptorSet);

        void SetPipeline(SR_HTYPES_NS::SharedPtr<Pipeline> pipeline) noexcept { m_pipeline = std::move(pipeline); }

    private:
        SR_NODISCARD DescriptorSet AllocateMemory(SR_GTYPES_NS::Shader* pShader) const;

    private:
        SR_HTYPES_NS::ObjectPool<std::vector<DescriptorSetInfo>, VirtualDescriptorSet> m_descriptorPool;
        SR_HTYPES_NS::SharedPtr<Pipeline> m_pipeline;

        mutable std::vector<DescriptorType> m_allocationTypesCache;

    };
}

#endif //SR_ENGINE_GRAPHICS_DESCRIPTOR_MANAGER_H
