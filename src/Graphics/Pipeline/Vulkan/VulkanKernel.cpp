//
// Created by Monika on 16.09.2023.
//

#include <Graphics/Pipeline/Pipeline.h>
#include <Graphics/Pipeline/Vulkan/VulkanKernel.h>
#include <Graphics/Pipeline/Vulkan/VulkanMemory.h>
#include <Graphics/Pipeline/Vulkan/VulkanPipeline.h>
#include <Graphics/Overlay/VulkanImGuiOverlay.h>
#include <Graphics/Window/Window.h>
#include <Graphics/Types/Framebuffer.h>

#include <Utils/Common/Features.h>

namespace SR_GRAPH_NS {
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

        auto&& prepareResult = PrepareFrame();
        switch (prepareResult) {
            case EvoVulkan::Core::FrameResult::Suboptimal:
                SRAssert2(!m_isSwapchainSuboptimal, "SRVulkan::Render() : suboptimal swapchain already set!");
                m_isSwapchainSuboptimal = true;
                break;
            case EvoVulkan::Core::FrameResult::OutOfDate: {
                VK_LOG("SRVulkan::Render() : out of date...");
                m_hasErrors |= !ReCreate(prepareResult);

                if (m_hasErrors) {
                    return EvoVulkan::Core::RenderResult::Fatal;
                }

                VK_LOG("SRVulkan::Render() : window are successfully resized!");

                return EvoVulkan::Core::RenderResult::Success;
            }
            case EvoVulkan::Core::FrameResult::Success:
                break;
            default:
                SRHalt("SRVulkan::Render() : unexcepted behaviour!");
                return EvoVulkan::Core::RenderResult::Error;
        }

        m_submitInfo.Clear();
        m_offscreenSubmitInfo.Clear();

        m_submitInfo.SetWaitDstStageMask(GetSubmitPipelineStages());
        m_offscreenSubmitInfo.SetWaitDstStageMask(GetSubmitPipelineStages());

        auto&& pVulkanPipeline = m_pipeline.DynamicCast<VulkanPipeline>();

        auto&& queues = m_pipeline->GetQueue().GetQueues();
        for (auto&& queue : queues) {
            for (auto&& pFrameBuffer : queue) {
                if (!pFrameBuffer->IsWasRendered()) {
                    continue;
                }

                if (!pFrameBuffer->IsValid()) {
                    SR_WARN("VulkanKernel::Render() : frame buffer is not valid! Skipping...");
                    continue;
                }

                auto&& pFBO = pVulkanPipeline->GetMemoryManager()->GetFBO(pFrameBuffer->GetId() - 1);

                if (pFrameBuffer->GetFeatures().offscreen) {
                    m_offscreenSubmitInfo.commandBuffers.emplace_back(pFBO->GetCommandBuffer(0));
                }
                else {
                    m_submitInfo.commandBuffers.emplace_back(pFBO->GetCommandBuffer(m_currentBuffer));
                }
            }
        }

        m_submitInfo.waitSemaphores.emplace_back(m_frameSyncs[m_currentBuffer].m_presentComplete);
        m_submitInfo.signalSemaphores.emplace_back(m_frameSyncs[m_currentBuffer].m_renderComplete);

        auto&& pImGuiOverlay = m_pipeline->GetOverlay(OverlayType::ImGui).DynamicCast<VulkanImGuiOverlay>();

        if (m_GUIEnabled && pImGuiOverlay && !pImGuiOverlay->IsSurfaceDirty()) {
            auto&& submitInfo = pImGuiOverlay->Render(m_currentBuffer);
            m_submitInfo.commandBuffers.emplace_back(submitInfo.commandBuffers.front());
        }
        else {
            if (m_pipeline->GetBuildState(m_currentBuffer).hasRenderData) {
                m_submitInfo.commandBuffers.emplace_back(m_drawCmdBuffs[m_currentBuffer]);
            }
        }

        if (!m_offscreenSubmitInfo.commandBuffers.empty())
        {
            SR_TRACY_ZONE_S("OffscreenGraphicsQueueSubmit");

            m_submitInfo.waitSemaphores.emplace_back(m_offscreenSemaphore);
            m_offscreenSubmitInfo.signalSemaphores.emplace_back(m_offscreenSemaphore);

            auto&& vkSubmitInfo = m_offscreenSubmitInfo.ToVk();

            if (auto&& result = vkQueueSubmit(m_device->GetQueues()->GetGraphicsQueue(), 1, &vkSubmitInfo, VK_NULL_HANDLE); result != VK_SUCCESS) {
                VK_ERROR("VulkanKernel::Render() : failed to offscreen queue submit! Reason: " + EvoVulkan::Tools::Convert::result_to_description(result));
                if (result == VK_ERROR_DEVICE_LOST) {
                    SR_PLATFORM_NS::Terminate();
                }
                return EvoVulkan::Core::RenderResult::Error;
            }

            WaitIdle();
        }

        {
            SR_TRACY_ZONE_S("GraphicsQueueSubmit");

            auto&& vkSubmitInfo = m_submitInfo.ToVk();
            vkResetFences(*m_device, 1, &m_waitFences[m_currentBuffer]);

            if (auto&& result = vkQueueSubmit(m_device->GetQueues()->GetGraphicsQueue(), 1, &vkSubmitInfo, m_waitFences[m_currentBuffer]); result != VK_SUCCESS) {
                VK_ERROR("VulkanKernel::Render() : failed to queue submit! Reason: " + EvoVulkan::Tools::Convert::result_to_description(result));
                if (result == VK_ERROR_DEVICE_LOST) {
                    SR_PLATFORM_NS::Terminate();
                }
                return EvoVulkan::Core::RenderResult::Error;
            }
        }

        EvoVulkan::Core::FrameResult presentResult = QueuePresent();

        //m_currentBuffer = (m_currentBuffer + 1) % GetSwapchainImagesCount();
        m_currentBuffer = (m_currentImage + 1) % GetSwapchainImagesCount();

        if (presentResult == EvoVulkan::Core::FrameResult::DeviceLost) {
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
        return Super::PrepareFrame();
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
        if (!m_pipeline) {
            return;
        }

        if (auto&& pWindow = m_pipeline->GetWindow()) {
            pWindow->PollEvents();
        }
    }

    bool VulkanKernel::SurfaceIsAvailable() const {
        SR_TRACY_ZONE;
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