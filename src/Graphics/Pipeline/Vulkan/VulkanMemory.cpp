//
// Created by Nikita on 15.06.2021.
//

#include <Graphics/Pipeline/Vulkan/VulkanMemory.h>
#include <Graphics/Pipeline/PipelineType.h>

#include <EvoVulkan/Types/VmaBuffer.h>

namespace SR_GRAPH_NS::VulkanTools {
    int32_t SR_GRAPH_NS::VulkanTools::MemoryManager::AllocateFBO(const VulkanFrameBufferAllocInfo& info) {
        if (info.inputColorAttachments.size() != info.pOutputColorAttachments->size()) {
            SR_WARN("MemoryManager::AllocateFBO() : input colors not equal output colors count! Something went wrong...");
        }

        info.pOutputColorAttachments->clear();

        VkImageAspectFlags vkImageAspect = VulkanTools::AbstractImageAspectToVkAspect(info.pDepth->aspect);
        VkFormat vkDepthFormat = m_device->GetDepthFormat();

        if (info.pDepth->format != ImageFormat::Auto) {
            vkDepthFormat = VulkanTools::AbstractTextureFormatToVkFormat(info.pDepth->format);
        }

        auto&& pFBO = EvoVulkan::Complexes::FrameBuffer::Create(
            m_kernel->GetDevice(),
            m_kernel->GetAllocator(),
            m_kernel->GetDescriptorManager(),
            m_kernel->GetSwapchain(),
            m_kernel->GetCmdPool(),
            info.features,
            info.inputColorAttachments,
            info.width, info.height,
            info.layersCount,
            1.f /** scale */,
            info.sampleCount,
            vkImageAspect,
            vkDepthFormat
        );

        if (!pFBO) {
            SR_ERROR("MemoryManager::AllocateFBO() : failed to create Evo Vulkan frame buffer object!");
            return SR_ID_INVALID;
        }

        for (auto&& pTexture : pFBO->AllocateColorTextureReferences()) {
            info.pOutputColorAttachments->emplace_back(m_texturePool.Add(pTexture));
        }

        if (info.pDepth->format != ImageFormat::None && info.pDepth->aspect != ImageAspect::None) {
            if (auto&& depthTexture = pFBO->AllocateDepthTextureReference(-1)) {
                info.pDepth->texture = m_texturePool.Add(depthTexture);
            }
        }

        for (auto&& pTexture : pFBO->AllocateDepthTextureReferences()) {
            info.pDepth->subLayers.emplace_back(m_texturePool.Add(pTexture));
        }

        return m_fboPool.Add(pFBO);
    }

    bool SR_GRAPH_NS::VulkanTools::MemoryManager::ReAllocateFBO(const VulkanFrameBufferAllocInfo& info) {
        auto&& vkImageAspect = VulkanTools::AbstractImageAspectToVkAspect(info.pDepth->aspect);

        auto&& pFBO = m_fboPool.At(info.FBO);
        SRAssert(pFBO);

        pFBO->SetSampleCount(info.sampleCount);
        pFBO->SetLayersCount(info.layersCount);
        pFBO->SetDepthAspect(vkImageAspect);
        pFBO->SetFeatures(info.features);

        if (!pFBO->ReCreate(info.width, info.height)) {
            SR_ERROR("MemoryManager::ReAllocateFBO() : failed to re-create frame buffer object!");
            return false;
        }

        /// Texture attachments

        auto&& textures = pFBO->AllocateColorTextureReferences();
        if (textures.size() != info.oldColorAttachments.size()) {
            SR_ERROR("MemoryManager::ReAllocateFBO() : incorrect old color attachments!");
            return false;
        }

        for (uint32_t i = 0; i < textures.size(); ++i) {
            EvoVulkan::Types::Texture*& pTextureRef = m_texturePool.At(info.oldColorAttachments[i]);
            delete pTextureRef;
            pTextureRef = textures[i];
        }

        /// Depth attachments

        auto depthTextures = pFBO->AllocateDepthTextureReferences();
        if (depthTextures.size() != info.pDepth->subLayers.size()) {
            SR_ERROR("MemoryManager::ReAllocateFBO() : incorrect old depth attachments!");
            return false;
        }

        for (uint32_t i = 0; i < depthTextures.size(); ++i) {
            EvoVulkan::Types::Texture*& pTextureRef = m_texturePool.At(info.pDepth->subLayers[i]);
            delete pTextureRef;
            pTextureRef = depthTextures[i];
        }

        /// Depth attachment

        if (info.pDepth->texture != SR_ID_INVALID) {
            EvoVulkan::Types::Texture*& pTextureRef = m_texturePool.At(info.pDepth->texture);
            delete pTextureRef;
            pTextureRef = pFBO->AllocateDepthTextureReference(-1);
        }

        return true;
    }

    bool SR_GRAPH_NS::VulkanTools::MemoryManager::FreeDescriptorSet(uint32_t id) {
        auto&& descriptorSet = m_descriptorSetPool.RemoveByIndex(static_cast<int32_t>(id));
        if (!m_descriptorManager->FreeDescriptorSet(&descriptorSet)){
            SR_ERROR("MemoryManager::FreeDescriptorSet() : failed free descriptor set!");
            return false;
        }
        return true;
    }

    bool MemoryManager::FreeSSBO(uint32_t id) {
        delete m_ssboPool.RemoveByIndex(static_cast<int32_t>(id));
        return true;
    }

    bool MemoryManager::FreeVBO(uint32_t id) {
        delete m_vboPool.RemoveByIndex(static_cast<int32_t>(id));
        return true;
    }

    bool MemoryManager::FreeUBO(uint32_t id) {
        delete m_uboPool.RemoveByIndex(static_cast<int32_t>(id));
        return true;
    }

    bool MemoryManager::FreeIBO(uint32_t id) {
        delete m_iboPool.RemoveByIndex(static_cast<int32_t>(id));
        return true;
    }

    bool MemoryManager::FreeFBO(uint32_t id) {
        delete m_fboPool.RemoveByIndex(static_cast<int32_t>(id));
        return true;
    }

    bool MemoryManager::FreeShaderProgram(uint32_t id) {
        delete m_shaderProgramPool.RemoveByIndex(static_cast<int32_t>(id));
        return true;
    }

    bool MemoryManager::FreeTexture(uint32_t id) {
        delete m_texturePool.RemoveByIndex(static_cast<int32_t>(id));
        return true;
    }

    int32_t MemoryManager::AllocateUBO(uint32_t UBOSize) {
        SR_TRACY_ZONE;

        auto&& pBuffer = EvoVulkan::Types::VmaBuffer::Create(
            m_kernel->GetAllocator(),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            UBOSize
        );

        if (!pBuffer) {
            SR_ERROR("MemoryManager::AllocateUBO() : failed to create uniform buffer object!");
            return SR_ID_INVALID;
        }

        return m_uboPool.Add(pBuffer);
    }

    int32_t MemoryManager::AllocateDescriptorSet(uint32_t shaderProgram, const std::vector<uint64_t>& types) {
        SR_TRACY_ZONE;

        auto&& pShaderProgram = m_shaderProgramPool.At(static_cast<int32_t>(shaderProgram));
        auto&& pLayout = pShaderProgram->GetDescriptorSetLayout();
        auto&& pDescriptorSet = m_descriptorManager->AllocateDescriptorSet(pLayout, types);

        if (!pDescriptorSet) {
            SR_ERROR("MemoryManager::AllocateDescriptorSet() : failed to allocate descriptor set!");
            return SR_ID_INVALID;
        }

        return m_descriptorSetPool.Add(pDescriptorSet);
    }

    int32_t MemoryManager::AllocateVBO(uint32_t buffSize, void *data) {
        SR_TRACY_ZONE;

        VkBufferUsageFlags bufferUsageFlagBits = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        if (m_kernel->GetDevice()->IsRayTracingSupported()) {
            bufferUsageFlagBits |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        }

        auto&& pVBO = EvoVulkan::Types::VmaBuffer::Create(
            m_kernel->GetAllocator(),
            bufferUsageFlagBits,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            buffSize, data
        );

        if (!pVBO) {
            SR_ERROR("MemoryManager::AllocateVBO() : failed to create vertex buffer object!");
            return SR_ID_INVALID;
        }

        return m_vboPool.Add(pVBO);
    }

    int32_t MemoryManager::AllocateIBO(uint32_t buffSize, void *data)  {
        SR_TRACY_ZONE;

        VkBufferUsageFlags bufferUsageFlagBits = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

        if (m_kernel->GetDevice()->IsRayTracingSupported()) {
            bufferUsageFlagBits |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        }

        auto&& pIBO = EvoVulkan::Types::VmaBuffer::Create(
            m_kernel->GetAllocator(),
            bufferUsageFlagBits,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            buffSize, data
        );

        if (!pIBO) {
            SR_ERROR("MemoryManager::AllocateIBO() : failed to create index buffer object!");
            return SR_ID_INVALID;
        }

        return m_iboPool.Add(pIBO);
    }

    int32_t MemoryManager::AllocateShaderProgram(EvoVulkan::Types::RenderPass renderPass)  {
        auto&& pShaderProgram = new EvoVulkan::Complexes::Shader(
            m_kernel->GetDevice(),
            renderPass,
            m_kernel->GetPipelineCache()
        );

        if (!pShaderProgram) {
            SR_ERROR("MemoryManager::AllocateShaderProgram() : failed to create shader program!");
            return SR_ID_INVALID;
        }

        return m_shaderProgramPool.Add(pShaderProgram);
    }

    int32_t MemoryManager::AllocateTexture(
            std::array<const uint8_t*, 6> pixels,
            uint32_t w,
            uint32_t h,
            VkFormat format,
            VkFilter /*filter*/,
            uint8_t mipLevels,
            bool cpuUsage)
    {
        auto&& pTexture = EvoVulkan::Types::Texture::LoadCubeMap(m_device, m_allocator, m_pool, format, w, h, pixels, mipLevels, cpuUsage);
        if (!pTexture) {
            SR_ERROR("MemoryManager::AllocateTexture() : failed to load Evo Vulkan texture!");
            return SR_ID_INVALID;
        }

        return m_texturePool.Add(pTexture);
    }

    int32_t MemoryManager::AllocateTexture(
        const uint8_t *pixels, uint32_t w, uint32_t h,
        VkFormat format,
        VkFilter filter,
        SR_GRAPH_NS::TextureCompression /*compression*/,
        uint8_t mipLevels,
        bool cpuUsage)
    {
        EvoVulkan::Types::Texture* pTexture = nullptr;

        if (mipLevels == 0) {
            pTexture = EvoVulkan::Types::Texture::LoadAutoMip(m_device, m_allocator, m_descriptorManager, m_pool, pixels, format, w, h, filter, cpuUsage);
        }
        else if (mipLevels == 1) {
            pTexture = EvoVulkan::Types::Texture::LoadWithoutMip(m_device, m_allocator, m_descriptorManager, m_pool, pixels, format, w, h, filter, cpuUsage);
        }
        else {
            pTexture = EvoVulkan::Types::Texture::Load(m_device, m_allocator, m_descriptorManager, m_pool, pixels, format, w, h, mipLevels, filter, cpuUsage);
        }

        if (!pTexture) {
            SR_ERROR("MemoryManager::AllocateTexture() : failed to load Evo Vulkan texture!");
            return SR_ID_INVALID;
        }

        return m_texturePool.Add(pTexture);
    }

    void SR_GRAPH_NS::VulkanTools::MemoryManager::Free() {
        SRAssert2(m_fboPool.IsEmpty(), "FBOs are not empty!");
        SRAssert2(m_uboPool.IsEmpty(), "UBOs are not empty!");
        SRAssert2(m_iboPool.IsEmpty(), "IBOs are not empty!");
        SRAssert2(m_vboPool.IsEmpty(), "VBOs are not empty!");
        SRAssert2(m_texturePool.IsEmpty(), "Textures are not empty!");
        SRAssert2(m_descriptorSetPool.IsEmpty(), "Descriptor sets are not empty!");
        SRAssert2(m_shaderProgramPool.IsEmpty(), "Shaders are not empty!");
        SRAssert2(m_ssboPool.IsEmpty(), "SSBOs are not empty!");
        delete this;
    }

    bool MemoryManager::Initialize(EvoVulkan::Core::VulkanKernel *kernel) {
        SR_TRACY_ZONE;

        if (m_isInit) {
            return false;
        }

        m_kernel = kernel;
        m_descriptorManager = m_kernel->GetDescriptorManager();
        m_allocator = m_kernel->GetAllocator();
        m_device = m_kernel->GetDevice();
        m_pool = m_kernel->GetCmdPool();

        if (!m_descriptorManager || !m_device || !m_pool) {
            SR_ERROR("MemoryManager::Initialize() : failed to get a (descriptor manager/device/cmd pool)!");
            return false;
        }

        m_isInit = true;
        return true;
    }

    int32_t MemoryManager::AllocateSSBO(uint32_t size, SSBOUsage usage) {
        SR_TRACY_ZONE;

        VmaMemoryUsage memoryUsage;

        switch (usage) {
            case SSBOUsage::Read:
                memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
                break;
            case SSBOUsage::Write:
                memoryUsage = VMA_MEMORY_USAGE_GPU_TO_CPU;
                break;
            case SSBOUsage::ReadWrite:
                memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
                break;
            default:
                SR_ERROR("MemoryManager::AllocateSSBO() : unknown usage!");
                return SR_ID_INVALID;
        }

        auto&& pBuffer = EvoVulkan::Types::VmaBuffer::Create(
            m_kernel->GetAllocator(),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            memoryUsage,
            size,
            /// TODO: я не уверен зачем это нужно
            VK_SHARING_MODE_EXCLUSIVE,
            static_cast<VkBufferCreateFlags>(0),
            /// TODO: я не уверен зачем это нужно
            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            nullptr
        );

        if (!pBuffer) {
            SR_ERROR("MemoryManager::AllocateSSBO() : failed to create storage buffer object!");
            return SR_ID_INVALID;
        }

        return m_ssboPool.Add(pBuffer);
    }
}
