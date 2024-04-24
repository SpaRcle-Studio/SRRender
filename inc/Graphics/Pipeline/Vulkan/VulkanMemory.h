//
// Created by Nikita on 27.05.2021.
//

#ifndef SR_ENGINE_GRAPHICS_VULKAN_MEMORY_H
#define SR_ENGINE_GRAPHICS_VULKAN_MEMORY_H

#include <Utils/Common/NonCopyable.h>

#include <EvoVulkan/Types/VulkanBuffer.h>
#include <EvoVulkan/Complexes/Framebuffer.h>
#include <EvoVulkan/VulkanKernel.h>
#include <EvoVulkan/Complexes/Shader.h>
#include <EvoVulkan/Types/Texture.h>
#include <EvoVulkan/DescriptorManager.h>
#include <EvoVulkan/Types/DescriptorSet.h>

#include <Graphics/Pipeline/TextureHelper.h>
#include <Graphics/Pipeline/Vulkan/DynamicTextureDescriptorSet.h>

namespace SR_GRAPH_NS::VulkanTools {
    struct VulkanFrameBufferAllocInfo {
        int32_t FBO = SR_ID_INVALID;
        uint32_t width = 0;
        uint32_t height = 0;
        DepthLayer* pDepth = nullptr;
        uint8_t sampleCount = 0;
        uint32_t layersCount = 0;
        std::vector<int32_t> oldColorAttachments;
        std::vector<VkFormat> inputColorAttachments;
        std::vector<int32_t>* pOutputColorAttachments = nullptr;
        EvoVulkan::Complexes::FrameBufferFeatures features;
    };

    class MemoryManager : SR_UTILS_NS::NonCopyable {
    private:
        MemoryManager() = default;
        ~MemoryManager() override = default;

    private:
        bool Initialize(EvoVulkan::Core::VulkanKernel* kernel);

    public:
        static MemoryManager* Create(EvoVulkan::Core::VulkanKernel* kernel) {
            auto memory = new MemoryManager();

            if (!memory->Initialize(kernel)) {
                SR_ERROR("MemoryManager::Create() : failed to initialize memory!");
                return nullptr;
            }

            return memory;
        }
        void Free();

    public:
        SR_NODISCARD bool FreeDescriptorSet(uint32_t id);

        SR_NODISCARD bool FreeShaderProgram(uint32_t id);

        SR_NODISCARD bool FreeVBO(uint32_t id);
        SR_NODISCARD bool FreeUBO(uint32_t id);
        SR_NODISCARD bool FreeIBO(uint32_t id);
        SR_NODISCARD bool FreeFBO(uint32_t id);
        SR_NODISCARD bool FreeSSBO(uint32_t id);

        SR_NODISCARD bool FreeTexture(uint32_t id);

    public:
        SR_NODISCARD int32_t AllocateDescriptorSet(uint32_t shaderProgram, const std::vector<uint64_t>& types);

        SR_NODISCARD int32_t AllocateShaderProgram(EvoVulkan::Types::RenderPass renderPass);

        SR_NODISCARD int32_t AllocateVBO(uint32_t buffSize, void* data);
        SR_NODISCARD int32_t AllocateUBO(uint32_t UBOSize);
        SR_NODISCARD int32_t AllocateIBO(uint32_t buffSize, void* data);
        SR_NODISCARD int32_t AllocateSSBO(uint32_t size, SSBOUsage usage);

        SR_NODISCARD bool ReAllocateFBO(const VulkanFrameBufferAllocInfo& info);

        SR_NODISCARD int32_t AllocateFBO(const VulkanFrameBufferAllocInfo& info);

        SR_NODISCARD int32_t AllocateTexture(
                const uint8_t* pixels,
                uint32_t w,
                uint32_t h,
                VkFormat format,
                VkFilter filter,
                TextureCompression compression,
                uint8_t mipLevels,
                bool cpuUsage);

        SR_NODISCARD int32_t AllocateTexture(
                std::array<const uint8_t*, 6> pixels,
                uint32_t w,
                uint32_t h,
                VkFormat format,
                VkFilter filter,
                uint8_t mipLevels,
                bool cpuUsage);

        SR_NODISCARD const EvoVulkan::Types::Texture* GetTexture(uint32_t id) const { return m_texturePool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD const EvoVulkan::Types::VmaBuffer* GetVBO(uint32_t id) const { return m_vboPool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD const EvoVulkan::Types::VmaBuffer* GetUBO(uint32_t id) const { return m_uboPool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD const EvoVulkan::Types::VmaBuffer* GetIBO(uint32_t id) const { return m_iboPool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD const EvoVulkan::Types::VmaBuffer* GetSSBO(uint32_t id) const { return m_ssboPool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD const EvoVulkan::Complexes::FrameBuffer* GetFBO(uint32_t id) const { return m_fboPool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD const EvoVulkan::Complexes::Shader* GetShaderProgram(uint32_t id) const { return m_shaderProgramPool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD const EvoVulkan::Types::DescriptorSet& GetDescriptorSet(uint32_t id) const { return m_descriptorSetPool.At(static_cast<int32_t>(id)); }

        SR_NODISCARD EvoVulkan::Types::Texture* GetTexture(uint32_t id) { return m_texturePool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD EvoVulkan::Types::VmaBuffer* GetVBO(uint32_t id) { return m_vboPool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD EvoVulkan::Types::VmaBuffer* GetUBO(uint32_t id) { return m_uboPool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD EvoVulkan::Types::VmaBuffer* GetIBO(uint32_t id) { return m_iboPool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD EvoVulkan::Types::VmaBuffer* GetSSBO(uint32_t id) { return m_ssboPool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD EvoVulkan::Complexes::FrameBuffer* GetFBO(uint32_t id) { return m_fboPool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD EvoVulkan::Complexes::Shader* GetShaderProgram(uint32_t id) { return m_shaderProgramPool.At(static_cast<int32_t>(id)); }
        SR_NODISCARD EvoVulkan::Types::DescriptorSet& GetDescriptorSet(uint32_t id) { return m_descriptorSetPool.At(static_cast<int32_t>(id)); }

        SR_NODISCARD bool IsTextureValid(uint32_t id) const { return m_texturePool.IsAlive(id); }

        void ForEachFBO(const SR_HTYPES_NS::Function<void(int32_t, EvoVulkan::Complexes::FrameBuffer*)>& func) const {
            m_fboPool.ForEach(func);
        }

    private:
        EvoVulkan::Core::DescriptorManager* m_descriptorManager = nullptr;
        EvoVulkan::Types::Device* m_device = nullptr;
        EvoVulkan::Memory::Allocator* m_allocator = nullptr;
        EvoVulkan::Types::CmdPool* m_pool = nullptr;

        SR_HTYPES_NS::ObjectPool<EvoVulkan::Types::DescriptorSet, int32_t> m_descriptorSetPool;
        SR_HTYPES_NS::ObjectPool<EvoVulkan::Complexes::Shader*, int32_t> m_shaderProgramPool;
        SR_HTYPES_NS::ObjectPool<EvoVulkan::Types::VmaBuffer*, int32_t> m_vboPool;
        SR_HTYPES_NS::ObjectPool<EvoVulkan::Types::VmaBuffer*, int32_t> m_uboPool;
        SR_HTYPES_NS::ObjectPool<EvoVulkan::Types::VmaBuffer*, int32_t> m_iboPool;
        SR_HTYPES_NS::ObjectPool<EvoVulkan::Types::VmaBuffer*, int32_t> m_ssboPool;
        SR_HTYPES_NS::ObjectPool<EvoVulkan::Complexes::FrameBuffer*, int32_t> m_fboPool;
        SR_HTYPES_NS::ObjectPool<EvoVulkan::Types::Texture*, int32_t> m_texturePool;

    private:
        bool m_isInit = false;
        EvoVulkan::Core::VulkanKernel* m_kernel = nullptr;

    };
}

#endif //SR_ENGINE_GRAPHICS_VULKAN_MEMORY_H
