// 
// Created by innerviewer on 2025-04-06.
//

#ifndef SR_RENDER_VULKAN_SKIA_OVERLAY_H
#define SR_RENDER_VULKAN_SKIA_OVERLAY_H

#include <Graphics/Overlay/SkiaOverlay.h>

#include <EvoVulkan/Tools/SubmitInfo.h>
#include <EvoVulkan/Types/Swapchain.h>

namespace SR_GRAPH_NS {
class VulkanSkiaOverlay : public SkiaOverlay {
        using Super = SkiaOverlay;
    public:
        explicit VulkanSkiaOverlay(PipelinePtr pPipeline)
            : Super(std::move(pPipeline))
        { }

        ~VulkanSkiaOverlay() override {
            SRAssert2(m_frameBuffs.empty(), "Vulkan ImGUI Overlay frame buffers are not empty");
            SRAssert2(m_cmdPools.empty(), "Vulkan ImGUI Overlay command pools are not empty");
            SRAssert2(m_cmdBuffs.empty(), "Vulkan ImGUI Overlay command buffers are not empty");
        }

    public:
        SR_NODISCARD bool Init() override;
        SR_NODISCARD bool ReCreate() override { return true; } // TODO: FIXME

        SR_NODISCARD std::string GetName() const override { return "Vulkan Skia"; }

        //SR_NODISCARD EvoVulkan::SubmitInfo& Render(uint32_t frame);
        //SR_NODISCARD void* GetTextureDescriptorSet(uint32_t textureId) override;

        SR_NODISCARD VkSemaphore GetSemaphore() { return m_semaphore; }
        SR_NODISCARD EvoVulkan::SubmitInfo& GetSubmitInfo() { return m_submitInfo; }

        //void ResetSubmitInfo();
        //void ReloadFonts() override;

        //void Destroy() override;
//
        //bool BeginDraw() override;
        //void EndDraw() override;

    private:
        bool InitializeRenderer();
        void DeInitializeRenderer();
        void DestroyBuffers();

        uint32_t GetCountImages() const;

    private:
        static const std::vector<VkDescriptorPoolSize> POOL_SIZES;

    private:
        EvoVulkan::SubmitInfo m_submitInfo = { };

        VkSemaphore m_semaphore = VK_NULL_HANDLE;

        VkCommandBufferBeginInfo m_cmdBuffBI = { };
        VkRenderPassBeginInfo m_renderPassBI = { };
        std::vector<VkClearValue> m_clearValues;

        std::vector<VkFramebuffer> m_frameBuffs;
        std::vector<VkCommandPool> m_cmdPools;
        std::vector<VkCommandBuffer> m_cmdBuffs;

        EvoVulkan::Types::DescriptorPool* m_pool = nullptr;
        EvoVulkan::Types::Device* m_device = nullptr;
        EvoVulkan::Types::Swapchain* m_swapChain = nullptr;
        EvoVulkan::Types::MultisampleTarget* m_multiSample = nullptr;

        bool m_undockingActive = false;

    };
} // namespace SR_GRAPH_NS

#endif //SR_RENDER_VULKAN_SKIA_OVERLAY_H
