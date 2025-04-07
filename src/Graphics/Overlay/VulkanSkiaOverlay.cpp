// 
// Created by innerviewer on 2025-04-06.
//

#include <Graphics/Overlay/VulkanSkiaOverlay.h>

#include <include/core/SkAlphaType.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>
#include <include/core/SkColorType.h>
#include <include/core/SkImageInfo.h>
#include <include/core/SkRefCnt.h>
#include <include/core/SkSurface.h>
#include <include/core/SkTypes.h>
#include <include/gpu/ganesh/GrDirectContext.h>
#include <include/gpu/ganesh/SkSurfaceGanesh.h>
#include <include/gpu/ganesh/vk/GrVkDirectContext.h>
#include <include/gpu/ganesh/vk/GrVkBackendSurface.h>
#include <include/gpu/ganesh/GrBackendSurface.h>
#include <include/gpu/ganesh/vk/GrVkTypes.h>
#include <include/gpu/vk/VulkanBackendContext.h>
#include <include/gpu/vk/VulkanExtensions.h>
#include <include/gpu/vk/VulkanMemoryAllocator.h>
#include <include/codec/SkCodec.h>

//void sk_abort_no_print() { SRAssert(false); }
//void SkDebugf(const char format[], ...) { SRAssert(false); }

namespace SR_GRAPH_NS {
    bool VulkanSkiaOverlay::Init() {
        auto&& vulkanKernel = m_pipeline.DynamicCast<VulkanPipeline>()->GetKernel();

        skgpu::VulkanBackendContext backendContext;

        backendContext.fDevice = *m_device;
        backendContext.fPhysicalDevice = *m_device;
        backendContext.fQueue = m_device->GetQueues()->GetGraphicsQueue();
        backendContext.fGraphicsQueueIndex = m_device->GetQueues()->GetGraphicsIndex();
        backendContext.fInstance = *m_device->GetInstance();

        auto&& pContext = GrDirectContexts::MakeVulkan(backendContext).release();

        auto&& imageInfo = EvoVulkan::Types::ImageCreateInfo(
             vulkanKernel->GetAllocator(),
             vulkanKernel->GetCmdPool(),
             m_swapChain->GetSurfaceWidth(),
             m_swapChain->GetSurfaceHeight(),
             1,
             VK_IMAGE_ASPECT_COLOR_BIT,
             VK_FORMAT_B8G8R8A8_UNORM,
             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
             1, // TODO: FIXME
             false,
             1,
             1
        );

        auto&& image = EvoVulkan::Types::Image::Create(imageInfo);

        GrVkImageInfo vkImageInfo;
        vkImageInfo.fImage = image;
        vkImageInfo.fImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        vkImageInfo.fFormat = VK_FORMAT_B8G8R8A8_UNORM; // The format of the Vulkan image
        vkImageInfo.fSampleCount = VK_SAMPLE_COUNT_1_BIT;
        vkImageInfo.fLevelCount = 1;

        auto&& renderTarget = GrBackendRenderTargets::MakeVk(m_swapChain->GetSurfaceWidth(), m_swapChain->GetSurfaceHeight(), vkImageInfo);

        SkSurfaceProps props(0, kUnknown_SkPixelGeometry);
        auto&& pSurface = SkSurfaces::WrapBackendRenderTarget(
            pContext,
            renderTarget,
            kTopLeft_GrSurfaceOrigin,
            kBGRA_8888_SkColorType,
            nullptr,
            &props                       // Optional surface properties
        ).release();

        SkCanvas* canvas = pSurface->getCanvas();
        canvas->clear(SK_ColorBLACK);

        // Example: Draw a red circle
        SkPaint paint;
        paint.setColor(SK_ColorRED);
        canvas->drawCircle(100, 100, 50, paint);

        // Finalize the frame
        pContext->flush();
        pContext->submit();          // Required: Actually pushes commands

        return true;
    }
}