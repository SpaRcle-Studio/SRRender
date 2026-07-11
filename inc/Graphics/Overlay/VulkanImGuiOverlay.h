//
// Created by Monika on 15.09.2023.
//

#ifndef SR_ENGINE_GRAPHICS_VULKAN_IMGUI_OVERLAY_H
#define SR_ENGINE_GRAPHICS_VULKAN_IMGUI_OVERLAY_H

#include <Graphics/Overlay/ImGuiOverlay.h>

#include <EvoVulkan/Tools/SubmitInfo.h>
#include <ImmediateGUI/Backend/PlatformBackend.h>
#include <ImmediateGUI/Backend/VulkanRenderer.h>

#include <unordered_map>

namespace EvoVulkan::Types {
    class Device;
    class Swapchain;
    class MultisampleTarget;
}

namespace SR_GRAPH_NS {
    class VulkanImGuiOverlay : public ImGuiOverlay {
        using Super = ImGuiOverlay;
    public:
        explicit VulkanImGuiOverlay(PipelinePtr pPipeline)
            : Super(std::move(pPipeline))
        { }

        ~VulkanImGuiOverlay() override {
            SRAssert2(m_vkRenderer == nullptr, "Vulkan ImGUI Overlay renderer is not destroyed");
        }

    public:
        SR_NODISCARD bool Init() override;
        SR_NODISCARD bool ReCreate() override;

        SR_NODISCARD std::string GetName() const override { return "Vulkan ImGUI"; }
        SR_NODISCARD bool IsDynamicRenderingEnabled() const override { return m_dynamicRendering; }

        SR_NODISCARD EvoVulkan::SubmitInfo& Render(uint32_t frame);
        SR_NODISCARD void* GetTextureDescriptorSet(uint32_t textureId) override;
        void OnTextureFreed(uint32_t textureId) override;

        SR_NODISCARD VkSemaphore GetSemaphore() { return m_semaphore; }
        SR_NODISCARD EvoVulkan::SubmitInfo& GetSubmitInfo() { return m_submitInfo; }

        void ResetSubmitInfo();
        void ReloadFonts() override;

        void Destroy() override;

        bool BeginDraw() override;
        void EndDraw() override;

    private:
        uint32_t GetCountImages() const;

    private:
        struct ImGuiTextureBinding {
            VkImageView view = VK_NULL_HANDLE;
            VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        };

        EvoVulkan::SubmitInfo m_submitInfo = { };

        bool m_dynamicRendering = false;

        VkSemaphore m_semaphore = VK_NULL_HANDLE;

        SR_GRAPH_GUI_NS::Immediate::PlatformBackend m_platformBackend = SR_GRAPH_GUI_NS::Immediate::PlatformBackend::None;
        SR_GRAPH_GUI_NS::Immediate::VulkanRendererHandle m_vkRenderer = nullptr;

        EvoVulkan::Types::Device* m_device = nullptr;
        EvoVulkan::Types::Swapchain* m_swapChain = nullptr;
        EvoVulkan::Types::MultisampleTarget* m_multiSample = nullptr;

        bool m_undockingActive = false;

        SR_HTYPES_NS::FlatHashMap<uint32_t, ImGuiTextureBinding> m_imguiTextureBindings;

    };
}

#endif //SR_ENGINE_GRAPHICS_VULKAN_IMGUI_OVERLAY_H
