//
// Created by Monika on 13.05.2023.
//

#ifndef SR_ENGINE_GRAPHICS_VULKAN_TRACY_H
#define SR_ENGINE_GRAPHICS_VULKAN_TRACY_H

#include <Graphics/macros.h>

#if defined(SR_TRACY_ENABLE) && defined(SR_USE_VULKAN) && defined(EVO_VULKAN_TRACY_GPU_PROFILER)
    #include <Utils/Profile/TracyContext.h>

    #include <EvoVulkan/VulkanKernel.h>
    #include <tracy/TracyVulkan.hpp>

    #define SR_TRACY_VK_CREATE(pKernel, name)                                                                           \
        auto&& pTracyVkCreateContext = SR_TRACY_GET_CONTEXT(SR_UTILS_NS::TracyType::Vulkan, (void*)nullptr);            \
        if (!pTracyVkCreateContext) {                                                                                   \
            pTracyVkCreateContext = (void*)TracyVkContext(                                                              \
                *pKernel->GetDevice(),                                                                                  \
                *pKernel->GetDevice(),                                                                                  \
                pKernel->GetDevice()->GetQueues()->GetGraphicsQueue(),                                                  \
                pKernel->GetDevice()->GetTracyVulkanCommandBuffer()                                                     \
            );                                                                                                          \
            if (pTracyVkCreateContext) {                                                                                \
                TracyVkContextName(((tracy::VkCtx*)pTracyVkCreateContext), name, (uint16_t)strlen(name));               \
            }                                                                                                           \
        }                                                                                                               \

    #define SR_TRACY_VK_FRAME_ZONE_N(cmd, name)                                                                         \
        auto&& pTracyVkFrameContext = SR_TRACY_GET_CONTEXT(SR_UTILS_NS::TracyType::Vulkan, (void*)nullptr);             \
        TracyVkNamedZone(                                                                                               \
            ((tracy::VkCtx*)pTracyVkFrameContext),                                                                      \
            TracyConcat(__tracy_vk_scope_, TracyLine),                                                                  \
            (VkCommandBuffer)cmd,                                                                                       \
            name,                                                                                                       \
            pTracyVkFrameContext != nullptr);

    #define SR_TRACY_VK_COLLECT(cmd)                                                                                    \
        auto&& pTracyVkCollectContext = SR_TRACY_GET_CONTEXT(SR_UTILS_NS::TracyType::Vulkan, (void*)nullptr);           \
        if (pTracyVkCollectContext) {                                                                                   \
            TracyVkCollect(((tracy::VkCtx*)pTracyVkCollectContext), (VkCommandBuffer)cmd);                              \
        }                                                                                                               \

#else
    #define SR_TRACY_VK_CREATE(pKernel, name)
    #define SR_TRACY_VK_FRAME_ZONE_N(cmd, name)
    #define SR_TRACY_VK_COLLECT(cmd)
#endif

#endif //SR_ENGINE_GRAPHICS_VULKAN_TRACY_H
