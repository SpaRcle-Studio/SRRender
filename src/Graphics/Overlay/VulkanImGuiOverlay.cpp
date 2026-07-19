//
// Created by Monika on 15.09.2023.
//

#include <Graphics/Overlay/VulkanImGuiOverlay.h>
#include <Graphics/Pipeline/Vulkan/VulkanPipeline.h>
#include <Graphics/Pipeline/Vulkan/VulkanKernel.h>
#include <Graphics/Pipeline/Vulkan/VulkanMemory.h>
#include <Graphics/Window/Window.h>

#if defined(SR_LINUX) && defined(SR_RENDER_GLFW)
    #include <Graphics/Window/GLFWWindow.h>
#endif

#if defined(SR_LINUX) && defined(SR_COMMON_USE_NATIVE_WAYLAND)
    #include <Graphics/Window/WaylandWindow.h>
#endif

#ifdef SR_ANDROID
    #include <Graphics/Window/AndroidWindow.h>
#endif

#include <ImmediateGUI/GUI/ImmediateGUI.h>

#include <Utils/Common/Features.h>
#include <Utils/Types/Time.h>

namespace SR_GRAPH_NS {
    bool VulkanImGuiOverlay::Init() {
        SR_TRACY_ZONE;

        auto&& pVulkanPipeline = SR_UTILS_NS::DynamicPointerCast<VulkanPipeline>(m_pipeline);
        auto&& pKernel = pVulkanPipeline->GetKernel();
        if (!pKernel->GetDevice() || !pKernel->GetDevice()->IsReady()) {
            SR_ERROR("VulkanImGuiOverlay::Init() : device is nullptr or not ready!");
            return false;
        }

        m_dynamicRendering = pKernel->GetDevice()->IsDynamicRenderingSupported();

        if (!Super::Init()) {
            return false;
        }

        m_pipeline->UpdateMultiSampling();
        m_tracyEnabled = SR_UTILS_NS::Features::Instance().Enabled("VulkanTracy", false);

        m_device = pKernel->GetDevice();
        m_swapChain = pKernel->GetSwapchain();
        m_multiSample = pKernel->GetMultisampleTarget();

        if (!m_device || !m_swapChain || !m_multiSample) {
            SR_ERROR("VulkanImGuiOverlay::Init() : device, multi sample or swapChain is nullptr!");
            return false;
        }

        auto&& pWindow = m_pipeline->GetWindow();

    #if defined(SR_WIN32)
        m_platformBackend = SR_GRAPH_GUI_NS::Immediate::PlatformBackend::Win32;
    #elif defined(SR_LINUX) && defined(SR_RENDER_GLFW)
        m_platformBackend = SR_GRAPH_GUI_NS::Immediate::PlatformBackend::GLFW;
    #elif defined(SR_LINUX) && defined(SR_COMMON_USE_NATIVE_WAYLAND)
        m_platformBackend = SR_GRAPH_GUI_NS::Immediate::PlatformBackend::WaylandCustom;
    #elif defined(SR_ANDROID)
        m_platformBackend = SR_GRAPH_GUI_NS::Immediate::PlatformBackend::Android;
    #else
        m_platformBackend = SR_GRAPH_GUI_NS::Immediate::PlatformBackend::None;
    #endif

        SR_GRAPH_GUI_NS::Immediate::PlatformInitInfo platformInit = { };
        platformInit.backend = m_platformBackend;
        platformInit.window = pWindow ? pWindow->GetHandle() : nullptr;

        if (!SR_GRAPH_GUI_NS::Immediate::PlatformInit(platformInit)) {
            SR_ERROR("VulkanImGuiOverlay::Init() : failed to initialize platform backend!");
            return false;
        }

        std::vector<SR_GRAPH_GUI_NS::Immediate::VulkanFrameInfo> frames;
        frames.resize(GetCountImages());
        for (uint32_t i = 0; i < frames.size(); ++i) {
            frames[i].image = m_swapChain->GetBuffers()[i].m_image;
            frames[i].imageView = m_swapChain->GetBuffers()[i].m_view;
            frames[i].width = m_swapChain->GetSurfaceWidth();
            frames[i].height = m_swapChain->GetSurfaceHeight();
        }

        SR_GRAPH_GUI_NS::Immediate::VulkanRendererCreateInfo vkInfo = { };
        vkInfo.instance = pKernel->GetInstance();
        vkInfo.physicalDevice = *pKernel->GetDevice();
        vkInfo.device = *pKernel->GetDevice();
        vkInfo.graphicsQueueFamily = static_cast<uint32_t>(pKernel->GetDevice()->GetQueues()->GetGraphicsIndex());
        vkInfo.graphicsQueue = pKernel->GetDevice()->GetQueues()->GetGraphicsQueue();
        vkInfo.pipelineCache = pKernel->GetPipelineCache();
        vkInfo.swapchainColorFormat = pKernel->GetSwapchain()->GetColorFormat();
        vkInfo.msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        vkInfo.enableDynamicRendering = m_dynamicRendering;
        vkInfo.enableViewports = IsViewportsEnabled();
        vkInfo.mainViewportPlatformHandleRaw = pWindow ? pWindow->GetHandle() : nullptr;

        if (!vkInfo.enableDynamicRendering && vkInfo.enableViewports) {
            SRHalt("Undocking requires dynamic rendering support!");
            return false;
        }

        m_vkRenderer = SR_GRAPH_GUI_NS::Immediate::VulkanCreateRenderer(vkInfo, frames.data(), static_cast<uint32_t>(frames.size()));
        if (!m_vkRenderer) {
            SR_ERROR("VulkanImGuiOverlay::Init() : failed to create vulkan imgui renderer!");
            return false;
        }

        m_semaphore = SR_GRAPH_GUI_NS::Immediate::VulkanGetRenderSemaphore(m_vkRenderer);

        if (!ReCreate()) {
            SR_ERROR("VulkanImGuiOverlay::Init() : failed to re-create!");
            return false;
        }

        m_initialized = true;
        return true;
    }

    void VulkanImGuiOverlay::Destroy() {
        if (m_vkRenderer) {
            // Release cached ImGui texture descriptor sets (allocated by imgui vulkan backend).
            for (auto&& [id, binding] : m_imguiTextureBindings) {
                if (binding.descriptorSet != VK_NULL_HANDLE) {
                    SR_GRAPH_GUI_NS::Immediate::VulkanRemoveTexture(m_vkRenderer, binding.descriptorSet);
                }
            }
            m_imguiTextureBindings.clear();
        }

        if (m_vkRenderer) {
            SR_GRAPH_GUI_NS::Immediate::VulkanDestroyRenderer(m_vkRenderer);
            m_vkRenderer = nullptr;
        }

        if (m_initialized) {
            SR_GRAPH_GUI_NS::Immediate::PlatformShutdown(m_platformBackend);
        }

        m_semaphore = VK_NULL_HANDLE;
        m_initialized = false;

        ImGuiOverlay::Destroy();
    }

    bool VulkanImGuiOverlay::BeginDraw() {
        SR_TRACY_ZONE;

        if (!m_context || !m_vkRenderer) {
            return false;
        }

        if (m_undockingActive != IsUndockingActive()) {
            SR_LOG("VulkanImGuiOverlay::BeginDraw() : undocking active changed!");
            m_undockingActive = IsUndockingActive();
            m_surfaceDirty = true;
            return false;
        }

        SR_GRAPH_GUI_NS::Immediate::VulkanNewFrame(m_vkRenderer);

        SR_GRAPH_GUI_NS::Immediate::PlatformNewFrameInfo frameInfo = { };

    #if defined(SR_LINUX) && defined(SR_COMMON_USE_NATIVE_WAYLAND)
        if (auto&& pNativeWindow = m_pipeline->GetWindow()->GetImplementation<WaylandWindow>()) {
            const float_t scale = pNativeWindow->GetScale();
            frameInfo.framebufferScale = { scale, scale };
            frameInfo.displaySize = { pNativeWindow->GetSurfaceWidth() / scale, pNativeWindow->GetSurfaceHeight() / scale };
        }
        frameInfo.deltaTime = SR_HTYPES_NS::Time::Instance().DeltaTime();
    #endif

        SR_GRAPH_GUI_NS::Immediate::PlatformNewFrame(m_platformBackend, frameInfo);
        SR_GRAPH_GUI_NS::Immediate::NewFrame();

        return true;
    }

    void VulkanImGuiOverlay::EndDraw() {
        SR_TRACY_ZONE;

        if (m_undockingActive != IsUndockingActive()) {
            SR_LOG("VulkanImGuiOverlay::EndDraw() : undocking active changed!");
            m_undockingActive = IsUndockingActive();
            m_surfaceDirty = true;

            SR_GRAPH_GUI_NS::Immediate::Render();

            if (IsViewportsEnabled()) {
                SR_GRAPH_GUI_NS::Immediate::UpdatePlatformWindows();
            }

            return;
        }

        SR_GRAPH_GUI_NS::Immediate::Render();

        if (IsViewportsEnabled()) {
            SR_GRAPH_GUI_NS::Immediate::UpdatePlatformWindows();
            SR_GRAPH_GUI_NS::Immediate::RenderPlatformWindowsDefault();
        }
    }

    bool VulkanImGuiOverlay::ReCreate() {
        SR_GRAPH_LOG("VulkanImGuiOverlay::ReCreate() : re-creating vulkan ImGui overlay...");

        if (!m_vkRenderer || !m_device || !m_swapChain) {
            SR_ERROR("VulkanImGuiOverlay::ReCreate() : renderer, device or swapChain is nullptr!");
            return false;
        }

        std::vector<SR_GRAPH_GUI_NS::Immediate::VulkanFrameInfo> frames;
        frames.resize(GetCountImages());
        for (uint32_t i = 0; i < frames.size(); ++i) {
            frames[i].image = m_swapChain->GetBuffers()[i].m_image;
            frames[i].imageView = m_swapChain->GetBuffers()[i].m_view;
            frames[i].width = m_swapChain->GetSurfaceWidth();
            frames[i].height = m_swapChain->GetSurfaceHeight();
        }

        if (!SR_GRAPH_GUI_NS::Immediate::VulkanRecreateRenderer(m_vkRenderer, frames.data(), static_cast<uint32_t>(frames.size()))) {
            SR_ERROR("VulkanImGuiOverlay::ReCreate() : failed to recreate vulkan renderer!");
            return false;
        }

        m_semaphore = SR_GRAPH_GUI_NS::Immediate::VulkanGetRenderSemaphore(m_vkRenderer);
        m_surfaceDirty = false;

        return true;
    }

    EvoVulkan::SubmitInfo& VulkanImGuiOverlay::Render(uint32_t frame) {
        SR_TRACY_ZONE_S("VulkanImGuiOverlay::Render");

        if (!m_vkRenderer) {
            return m_submitInfo;
        }

        VkCommandBuffer cmd = SR_GRAPH_GUI_NS::Immediate::VulkanRecordFrame(m_vkRenderer, frame, m_tracyEnabled);
        if (cmd == VK_NULL_HANDLE) {
            return m_submitInfo;
        }

        m_submitInfo.commandBuffers.clear();
        m_submitInfo.commandBuffers.emplace_back(cmd);

        return m_submitInfo;
    }

    void* VulkanImGuiOverlay::GetTextureDescriptorSet(uint32_t textureId) {
        if (textureId == SR_ID_INVALID) {
            SR_ERROR("VulkanImGuiOverlay::GetTextureDescriptorSet() : invalid id!");
            return nullptr;
        }

        if (!m_context || !m_vkRenderer) {
            SR_ERROR("VulkanImGuiOverlay::GetTextureDescriptorSet() : ImGui is not initialized!");
            return nullptr;
        }

        auto&& pMemoryManager = SR_UTILS_NS::DynamicPointerCast<VulkanPipeline>(m_pipeline)->GetMemoryManager();

        auto* pTexture = pMemoryManager ? pMemoryManager->GetTexture(textureId) : nullptr;
        if (!pTexture) {
            return nullptr;
        }

        const VkImageView view = pTexture->GetImageView();
        const VkImageLayout layout = pTexture->GetLayout();
        if (view == VK_NULL_HANDLE) {
            return nullptr;
        }

        auto& binding = m_imguiTextureBindings[textureId];
        if (binding.descriptorSet != VK_NULL_HANDLE && binding.view == view && binding.layout == layout) {
            return reinterpret_cast<void*>(binding.descriptorSet);
        }

        if (binding.descriptorSet != VK_NULL_HANDLE) {
            SR_GRAPH_GUI_NS::Immediate::VulkanRemoveTexture(m_vkRenderer, binding.descriptorSet);
            binding = ImGuiTextureBinding{};
        }

        binding.view = view;
        binding.layout = layout;
        binding.descriptorSet = SR_GRAPH_GUI_NS::Immediate::VulkanAddTexture(m_vkRenderer, view, layout);

        if (binding.descriptorSet == VK_NULL_HANDLE) {
            binding = ImGuiTextureBinding{};
            return nullptr;
        }

        return reinterpret_cast<void*>(binding.descriptorSet);
    }

    void VulkanImGuiOverlay::OnTextureFreed(uint32_t textureId) {
        auto it = m_imguiTextureBindings.find(textureId);
        if (it == m_imguiTextureBindings.end()) {
            return;
        }

        if (m_vkRenderer && it->second.descriptorSet != VK_NULL_HANDLE) {
            SR_GRAPH_GUI_NS::Immediate::VulkanRemoveTexture(m_vkRenderer, it->second.descriptorSet);
        }

        m_imguiTextureBindings.erase(it);
    }

    uint32_t VulkanImGuiOverlay::GetCountImages() const {
        return m_swapChain ? m_swapChain->GetCountImages() : 0;
    }

    void VulkanImGuiOverlay::ResetSubmitInfo() {
        auto&& pKernel = SR_UTILS_NS::DynamicPointerCast<VulkanPipeline>(m_pipeline)->GetKernel();
        if (!pKernel) {
            SR_ERROR("VulkanImGuiOverlay::ResetSubmitInfo() : kernel is nullptr!");
            return;
        }

        m_submitInfo = { };
        m_submitInfo.SetWaitDstStageMask(pKernel->GetSubmitPipelineStages());
        m_submitInfo.AddSignalSemaphore(m_semaphore);
    }

    void VulkanImGuiOverlay::ReloadFonts() {
        Super::ReloadFonts();
        SR_GRAPH_GUI_NS::Immediate::VulkanReloadFonts(m_vkRenderer);
    }
}

