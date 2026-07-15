#include <Graphics/macros.h>

#if defined(SR_USE_VULKAN)
    #include "../src/Graphics/Pipeline/Vulkan/VulkanPipeline.cpp"
    #include "../src/Graphics/Pipeline/Vulkan/VulkanMemory.cpp"
    #include "../src/Graphics/Pipeline/Vulkan/VulkanKernel.cpp"

    #if defined(SR_LINUX)
        #include "../src/Graphics/Pipeline/Vulkan/X11SurfaceInit.cpp"
    #endif

    #include "../src/Graphics/Overlay/VulkanImGuiOverlay.cpp"

    #if defined(SR_TRACY_ENABLE)
        #include "../src/Graphics/Pipeline/Vulkan/VulkanTracy.cpp"
    #endif
#endif

#if defined(SR_RENDER_USE_WEBGPU)
    #include "../src/Graphics/Pipeline/WebGPU/WebGPUPipeline.cpp"
    #include "../src/Graphics/Overlay/WebGPUImGuiOverlay.cpp"
#endif

#if defined(SR_EMSCRIPTEN)
    #include "../src/Graphics/Window/EmscriptenWindow.cpp"
#endif

#if defined(SR_WIN32)
    #include "../src/Graphics/Window/Win32Window.cpp"
#endif

#if defined(SR_ANDROID)
    #include "../src/Graphics/Window/AndroidWindow.cpp"
#endif

#if defined(SR_LINUX)
    #include "../src/Graphics/Window/GLFWWindow.cpp"
#endif