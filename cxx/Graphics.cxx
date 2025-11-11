#include <Graphics/macros.h>

#if defined(SR_USE_IMGUI)
    #include "../src/Graphics/Overlay/ImGuiOverlay.cpp"
#endif

#if defined(SR_USE_VULKAN)
    #include "../src/Graphics/Pipeline/Vulkan/VulkanPipeline.cpp"
    #include "../src/Graphics/Pipeline/Vulkan/VulkanMemory.cpp"
    #include "../src/Graphics/Pipeline/Vulkan/VulkanKernel.cpp"

    #if defined(SR_LINUX)
        #include "../src/Graphics/Pipeline/Vulkan/X11SurfaceInit.cpp"
    #endif

    #if defined(SR_USE_IMGUI)
        #include "../src/Graphics/Overlay/VulkanImGuiOverlay.cpp"
    #endif

    #if defined(SR_TRACY_ENABLE)
        #include "../src/Graphics/Pipeline/Vulkan/VulkanTracy.cpp"
    #endif
#endif

#if defined(SR_WIN32)
    #include "../src/Graphics/Window/Win32Window.cpp"
#endif

#if defined(SR_ANDROID)
    #include "../src/Graphics/Window/AndroidWindow.cpp"
#endif

#if defined(SR_LINUX)
    //#include "../src/Graphics/Window/X11Window.cpp"
    #include "../src/Graphics/Window/GLFWWindow.cpp"
#endif