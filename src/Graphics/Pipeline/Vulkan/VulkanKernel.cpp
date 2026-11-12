//
// Created by Monika on 16.09.2023.
//

#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Pipeline/Vulkan/VulkanKernel.h>
#include <Graphics/Pipeline/Vulkan/VulkanMemory.h>
#include <Graphics/Pipeline/Vulkan/VulkanPipeline.h>
#include <Graphics/Overlay/VulkanImGuiOverlay.h>
#include <Graphics/Window/Window.h>
#include <Graphics/Window/BasicWindowImpl.h>
#include <Graphics/Types/Framebuffer.h>

#ifdef SR_ANDROID
    #include <Graphics/Window/AndroidWindow.h>
    #include <EvoVulkan/Types/Surface.h>
    #include <EvoVulkan/Types/Swapchain.h>
    #include <Utils/Platform/AndroidLifecycle.h>
#endif

#include <Utils/Common/Features.h>

namespace SR_GRAPH_NS {
#ifdef SR_ANDROID
    bool VulkanKernel::ReCreate(EvoVulkan::Core::FrameResult reason) {
        if (reason == EvoVulkan::Core::FrameResult::OutOfDate || reason == EvoVulkan::Core::FrameResult::Suboptimal) {
            PollWindowEvents();
            const auto wrapper = m_pipeline->GetWindow();
            const auto window = wrapper ? wrapper->GetImplementation<AndroidWindow>() : nullptr;
            if (!window || !window->IsSurfaceAvailable()) {
                m_hasAcquiredImage = false;
                return true;
            }
            if (reason == EvoVulkan::Core::FrameResult::Suboptimal &&
                m_surface->GetHandle() == window->GetNativeWindow())
            {
                if (ANativeWindow_getWidth(window->GetNativeWindow()) == m_swapchain->GetSurfaceWidth() &&
                    ANativeWindow_getHeight(window->GetNativeWindow()) == m_swapchain->GetSurfaceHeight())
                {
                    // Compositor rotation can keep an otherwise usable swapchain suboptimal on Android.
                    return true;
                }
            }
            return ChangeAndroidSurface(window->GetNativeWindow());
        }
        if (m_newWidth > 0 && m_newHeight > 0) {
            m_width = m_newWidth;
            m_height = m_newHeight;
            m_newWidth = -1;
            m_newHeight = -1;
        }
        return Super::ReCreate(reason);
    }

    bool VulkanKernel::ChangeAndroidSurface(void* handle) {
        if (!m_device || !m_surface || !m_swapchain) {
            return false;
        }
        if (vkDeviceWaitIdle(*m_device) != VK_SUCCESS) {
            return false;
        }
        const auto overlay = SR_UTILS_NS::DynamicPointerCast<VulkanImGuiOverlay>(m_pipeline->GetOverlay(OverlayType::ImGui));
        if (overlay) {
            overlay->SuspendNativeSurface();
        }
        m_hasAcquiredImage = false;
        m_isSwapchainSuboptimal = false;
        m_frameIndex = 0;
        m_imageIndex = 0;
        for (auto framebuffer : m_frameBuffers) {
            vkDestroyFramebuffer(*m_device, framebuffer, nullptr);
        }
        m_frameBuffers.clear();
        const auto oldFormat = m_swapchain->GetColorFormat();
        m_swapchain->ReleaseSurface();
        if (!m_surface->Replace(VK_NULL_HANDLE, nullptr, m_device)) {
            return false;
        }
        if (!handle) {
            return true;
        }
        auto window = static_cast<ANativeWindow*>(handle);
        const int32_t width = ANativeWindow_getWidth(window);
        const int32_t height = ANativeWindow_getHeight(window);
        if (width <= 0 || height <= 0) {
            return false;
        }
        VkAndroidSurfaceCreateInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        info.window = window;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (vkCreateAndroidSurfaceKHR(*m_instance, &info, nullptr, &surface) != VK_SUCCESS) {
            return false;
        }
        if (!m_surface->Replace(surface, window, m_device)) {
            return false;
        }
        m_width = width;
        m_height = height;
        m_newWidth = -1;
        m_newHeight = -1;
        if (!m_swapchain->ResumeSurface(m_width, m_height, m_requiredSwapchainImages) || m_swapchain->GetColorFormat() != oldFormat) {
            SR_ERROR("VulkanKernel::ChangeAndroidSurface() : unsupported surface format or failed swapchain recreation.");
            return false;
        }
        if (overlay && !overlay->ResumeNativeSurface(window)) {
            return false;
        }
        return ReCreate(EvoVulkan::Core::FrameResult::Dirty);
    }
#endif

    VulkanKernel::VulkanKernel(VulkanKernel::PipelinePtr pPipeline)
        : Super()
        , m_pipeline(std::move(pPipeline))
    { }

    bool VulkanKernel::OnResize()  {
        vkQueueWaitIdle(m_device->GetQueues()->GetGraphicsQueue());
        vkDeviceWaitIdle(*m_device);

        if (m_pipeline) {
            m_pipeline->SetDirty(true);
            if (m_GUIEnabled) {
                m_pipeline->ReCreateOverlay();
            }
        }

        return true;
    }

    bool VulkanKernel::IsWindowValid() const {
        if (!m_pipeline) {
            return false;
        }

        if (m_pipeline->GetWindow() && m_pipeline->GetWindow()->IsValid()) {
            return true;
        }

        return false;

        //return m_pipeline->GetWindow().Do<bool>([](Window* pWindow) -> bool {
        //    return pWindow->IsValid();
        //}, false);
    }

    void VulkanKernel::SetGUIEnabled(bool enabled) {
        if (m_pipeline) {
            m_pipeline->SetOverlaySurfaceDirty();
        }
        Super::SetGUIEnabled(enabled);
    }

    EvoVulkan::Core::RenderResult VulkanKernel::Render()  {
        SR_TRACY_ZONE;

        if (!SurfaceIsAvailable()) {
            return EvoVulkan::Core::RenderResult::Success;
        }

        if (!m_hasAcquiredImage) {
            return EvoVulkan::Core::RenderResult::Success;
        }

        m_submitInfo.Clear();
        m_submitInfo.SetWaitDstStageMask(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

        FrameSync& frame = m_frames[m_frameIndex];

        vkResetFences(*m_device, 1, &frame.inFlightFence);

        auto&& pVulkanPipeline = SR_UTILS_NS::DynamicPointerCast<VulkanPipeline>(m_pipeline);

        bool hasDrawCommandBuffer = false;
        for (uint32_t cmdBufferId : m_pipeline->GetCmdBuffersQueue()) {
            if (cmdBufferId == SR_ID_INVALID) {
                m_submitInfo.commandBuffers.emplace_back(m_drawCmdBuffs[m_imageIndex]);
                hasDrawCommandBuffer = true;
            }
            else {
                SRHalt("VulkanKernel::Render() : TODO: support custom command buffers!");
            }
        }

        if (!hasDrawCommandBuffer) {
            m_submitInfo.commandBuffers.emplace_back(m_drawCmdBuffs[m_imageIndex]);
        }

        m_submitInfo.signalSemaphores.emplace_back(m_renderFinished[m_imageIndex]);
        m_submitInfo.waitSemaphores.emplace_back(frame.imageAvailable);

        auto&& pImGuiOverlay = SR_UTILS_NS::DynamicPointerCast<VulkanImGuiOverlay>(m_pipeline->GetOverlay(OverlayType::ImGui));

        if (m_GUIEnabled && pImGuiOverlay && !pImGuiOverlay->IsSurfaceDirty()) {
            auto&& submitInfo = pImGuiOverlay->Render(m_imageIndex);
            m_submitInfo.commandBuffers.emplace_back(submitInfo.commandBuffers.front());
        }
        //else {
        //    m_submitInfo.commandBuffers.emplace_back(m_drawCmdBuffs[m_imageIndex]);
        //}

        {
            SR_TRACY_ZONE_S("GraphicsQueueSubmit");

            auto&& vkSubmitInfo = m_submitInfo.ToVk();

            if (auto&& result = vkQueueSubmit(m_device->GetQueues()->GetGraphicsQueue(), 1, &vkSubmitInfo, frame.inFlightFence); result != VK_SUCCESS) {
                VK_ERROR("VulkanKernel::Render() : failed to queue submit! Reason: " + EvoVulkan::Tools::Convert::result_to_description(result));
                if (result == VK_ERROR_DEVICE_LOST) {
                    m_device->OnDeviceLost();
                    SR_PLATFORM_NS::Terminate();
                }
                return EvoVulkan::Core::RenderResult::Error;
            }
        }

        const bool isSwapchainSuboptimal = m_isSwapchainSuboptimal;
        EvoVulkan::Core::FrameResult presentResult = QueuePresent();
        if (isSwapchainSuboptimal && presentResult == EvoVulkan::Core::FrameResult::Success) {
            presentResult = EvoVulkan::Core::FrameResult::Suboptimal;
        }
        m_frameIndex = (m_frameIndex + 1) % GetMaxFramesInFlight();

        if (presentResult == EvoVulkan::Core::FrameResult::DeviceLost) {
            m_device->OnDeviceLost();
            SR_PLATFORM_NS::Terminate();
        }

        if (m_isSwapchainSuboptimal || presentResult == EvoVulkan::Core::FrameResult::OutOfDate || presentResult == EvoVulkan::Core::FrameResult::Suboptimal) {
            m_hasErrors |= !ReCreate(m_isSwapchainSuboptimal ? EvoVulkan::Core::FrameResult::Suboptimal : presentResult);
            m_isSwapchainSuboptimal = false;

            if (m_hasErrors) {
                return EvoVulkan::Core::RenderResult::Fatal;
            }
        }

        switch (presentResult) {
            case EvoVulkan::Core::FrameResult::Success:
                return EvoVulkan::Core::RenderResult::Success;

            case EvoVulkan::Core::FrameResult::Error:
                return EvoVulkan::Core::RenderResult::Error;

            case EvoVulkan::Core::FrameResult::OutOfDate:
            case EvoVulkan::Core::FrameResult::Suboptimal:
                return EvoVulkan::Core::RenderResult::Success;

            default: {
                SRAssertOnce(false);
                return EvoVulkan::Core::RenderResult::Fatal;
            }
        }
    }

    bool VulkanKernel::IsRayTracingRequired() const noexcept {
    #ifdef SR_ANDROID
        return false;
    #else
        return SR_UTILS_NS::Features::Instance().Enabled("RayTracing", false);
    #endif
    }

    EvoVulkan::Core::FrameResult VulkanKernel::PrepareFrame() {
        SR_TRACY_ZONE;

        m_hasAcquiredImage = false;
    #ifdef SR_ANDROID
        if (m_pipeline->GetWindow() && !m_pipeline->GetWindow()->GetBaseWindow()->IsSurfaceAvailable()) {
            return EvoVulkan::Core::FrameResult::Success;
        }
        if (m_newWidth > 0 && m_newHeight > 0 &&
            (static_cast<uint32_t>(m_newWidth) != m_swapchain->GetSurfaceWidth() ||
                static_cast<uint32_t>(m_newHeight) != m_swapchain->GetSurfaceHeight()))
        {
            if (!ReCreate(EvoVulkan::Core::FrameResult::Dirty)) {
                m_hasErrors = true;
                return EvoVulkan::Core::FrameResult::Fatal;
            }
        }
    #endif
        auto prepareResult = Super::PrepareFrame();
        switch (prepareResult) {
            case EvoVulkan::Core::FrameResult::None:
                return prepareResult;
            case EvoVulkan::Core::FrameResult::Suboptimal:
                //SRAssert2(!m_isSwapchainSuboptimal, "SRVulkan::Render() : suboptimal swapchain already set!");
                m_isSwapchainSuboptimal = true;
                break;
            case EvoVulkan::Core::FrameResult::OutOfDate: {
                VK_LOG("SRVulkan::Render() : out of date...");
                m_hasErrors |= !ReCreate(prepareResult);

                if (m_hasErrors) {
                    return EvoVulkan::Core::FrameResult::Fatal;
                }

                VK_LOG("SRVulkan::Render() : window are successfully resized!");
                if (!SurfaceIsAvailable()) {
                    return EvoVulkan::Core::FrameResult::None;
                }
                prepareResult = Super::PrepareFrame();
                if (prepareResult == EvoVulkan::Core::FrameResult::Suboptimal) {
                    m_isSwapchainSuboptimal = true;
                }
                else if (prepareResult != EvoVulkan::Core::FrameResult::Success) {
                    return prepareResult;
                }
                break;
            }
            case EvoVulkan::Core::FrameResult::Success:
                break;
            case EvoVulkan::Core::FrameResult::DeviceLost:
                m_device->OnDeviceLost();
                SR_PLATFORM_NS::Terminate();
            default:
                SRHalt("SRVulkan::Render() : unexcepted behaviour!");
                return EvoVulkan::Core::FrameResult::Error;
        }

        m_hasAcquiredImage = prepareResult == EvoVulkan::Core::FrameResult::Success ||
            prepareResult == EvoVulkan::Core::FrameResult::Suboptimal;
        return prepareResult;
    }

    EvoVulkan::Core::FrameResult VulkanKernel::SubmitFrame() {
        SR_TRACY_ZONE;
        return Super::SubmitFrame();
    }

    EvoVulkan::Core::FrameResult VulkanKernel::QueuePresent() {
        SR_TRACY_ZONE;
        SR_TRACY_ZONE_COLOR(0x008800);
        return Super::QueuePresent();
    }

    EvoVulkan::Core::FrameResult VulkanKernel::WaitIdle() {
        SR_TRACY_ZONE;
        return Super::WaitIdle();
    }

    void VulkanKernel::PollWindowEvents() {
        SR_TRACY_ZONE;

        if (!m_pipeline) {
            return;
        }

        if (auto&& pWindow = m_pipeline->GetWindow()) {
            pWindow->PollEvents();
        }
    }

    bool VulkanKernel::SurfaceIsAvailable() const {
        SR_TRACY_ZONE;

        if (!m_swapchain) {
            return false;
        }

        //return m_swapchain->SurfaceIsAvailable();
        return m_swapchain->GetSurfaceHeight() > 0 && m_swapchain->GetSurfaceWidth() > 0;
    }

    void VulkanKernel::WaitFences() {
        SR_TRACY_ZONE;
        SR_TRACY_ZONE_COLOR(0x0000FF);
        Super::WaitFences();
    }

    void VulkanKernel::WaitAllFences() {
        SR_TRACY_ZONE;
        SR_TRACY_ZONE_COLOR(0xFF0000);
        Super::WaitAllFences();
    }

    void VulkanKernel::WaitDeviceIdle() {
        SR_TRACY_ZONE;
        SR_TRACY_ZONE_COLOR(0xFF0000);
        Super::WaitDeviceIdle();
    }
}
