//
// Created by innerviewer on 07/11/23.
//

#include <Graphics/Pipeline/Vulkan/X11SurfaceInit.h>

#include <X11/Xlib-xcb.h>

#include <vulkan/vulkan_xcb.h>
#include <vulkan/vulkan_xlib.h>

namespace SR_GRAPH_NS {
    VkSurfaceKHR X11SurfaceInit::Init(const SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::Window>& window, VkInstance instance) {
        if (auto&& pImpl = window->GetImplementation<X11Window>()) {
            VkXcbSurfaceCreateInfoKHR surfaceInfo = { };
            surfaceInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            surfaceInfo.pNext = nullptr;
            surfaceInfo.flags = 0;
            surfaceInfo.connection = pImpl->GetConnection();
            surfaceInfo.window = pImpl->GetWindow();

            VkSurfaceKHR surface = VK_NULL_HANDLE;
            VkResult result = vkCreateXcbSurfaceKHR(instance, &surfaceInfo, nullptr, &surface);
            if (result != VK_SUCCESS) {
                VK_ERROR("X11SurfaceInit::Init() : failed to create X11 surface. \n\tReason: " +
                    EvoVulkan::Tools::Convert::result_to_string(result) +
                    "\n\tDescription: " + EvoVulkan::Tools::Convert::result_to_description(result));
                return VK_NULL_HANDLE;
            }
                return surface;
        }
        else {
            SR_ERROR("X11PipelineInit::Init() : window does not support this architecture!");
            return VK_NULL_HANDLE;
        }

        /*if (auto&& pImpl = window->GetImplementation<X11Window>()) {
            VkXlibSurfaceCreateInfoKHR surfaceInfo = { };
            surfaceInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            surfaceInfo.pNext = nullptr;
            surfaceInfo.flags = 0;
            surfaceInfo.dpy = static_cast<Display*>(pImpl->GetDisplay());
            surfaceInfo.window = pImpl->GetWindow();

            VkSurfaceKHR surface = VK_NULL_HANDLE;
            VkResult result = vkCreateXlibSurfaceKHR(instance, &surfaceInfo, nullptr, &surface);
            if (result != VK_SUCCESS) {
                VK_ERROR("X11SurfaceInit::Init() : failed to create X11 surface. \n\tReason: " +
                    EvoVulkan::Tools::Convert::result_to_string(result) +
                    "\n\tDescription: " + EvoVulkan::Tools::Convert::result_to_description(result));
                return VK_NULL_HANDLE;
            }
            else
                return surface;
        }
        else {
            SR_ERROR("X11PipelineInit::Init() : window does not support this architecture!");
            return VK_NULL_HANDLE;
        }*/
    }

    const char* X11SurfaceInit::GetSurfaceExtensionName() {
        return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
    }
}