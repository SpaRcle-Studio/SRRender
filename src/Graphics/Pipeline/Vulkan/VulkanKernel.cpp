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

        m_submitInfo.Clear();
        m_submitInfo.SetWaitDstStageMask(GetSubmitPipelineStages());

        FrameSync& frame = m_frames[m_frameIndex];

        vkResetFences(*m_device, 1, &frame.inFlightFence);

        auto&& pVulkanPipeline = SR_UTILS_NS::DynamicPointerCast<VulkanPipeline>(m_pipeline);

        //auto&& queues = m_pipeline->GetQueue().GetQueues();
        //for (auto&& queue : queues) {
        //    for (auto&& pFrameBuffer : queue) {
        //        if (!pFrameBuffer->IsWasRendered()) {
        //            continue;
        //        }

        //        if (!pFrameBuffer->IsValid()) {
        //            continue;
        //        }

        //        auto&& pFBO = pVulkanPipeline->GetMemoryManager()->GetFBO(pFrameBuffer->GetId() - 1);

        //        m_submitInfo.commandBuffers.emplace_back(pFBO->GetCommandBuffer(m_imageIndex));
        //    }
        //}

        for (uint32_t cmdBufferId : m_pipeline->GetCmdBuffersQueue()) {
            if (cmdBufferId == SR_ID_INVALID) {
                m_submitInfo.commandBuffers.emplace_back(m_drawCmdBuffs[m_imageIndex]);
            }
            else {
                SRHalt("VulkanKernel::Render() : TODO: support custom command buffers!");
            }
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
                    SR_PLATFORM_NS::Terminate();
                }
                return EvoVulkan::Core::RenderResult::Error;
            }
        }

        EvoVulkan::Core::FrameResult presentResult = m_isSwapchainSuboptimal ? EvoVulkan::Core::FrameResult::Suboptimal : QueuePresent();
        m_frameIndex = (m_frameIndex + 1) % GetMaxFramesInFlight();

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

        auto&& prepareResult = Super::PrepareFrame();
        switch (prepareResult) {
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

                return EvoVulkan::Core::FrameResult::Success;
            }
            case EvoVulkan::Core::FrameResult::Success:
                break;
            default:
                SRHalt("SRVulkan::Render() : unexcepted behaviour!");
                return EvoVulkan::Core::FrameResult::Error;
        }

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