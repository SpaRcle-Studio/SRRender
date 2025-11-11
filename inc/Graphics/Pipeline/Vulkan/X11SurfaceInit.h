//
// Created by innerviewer on 07/11/23.
//

#ifndef SR_ENGINE_X11SURFACEINIT_H
#define SR_ENGINE_X11SURFACEINIT_H

#include <Graphics/macros.h>

#include <EvoVulkan/macros.h>

#include <Utils/Types/SharedPtr.h>

namespace SR_GRAPH_NS {
    class Window;

    class X11SurfaceInit {
    public:
        static VkSurfaceKHR Init(const SR_HTYPES_NS::SharedPtr<SR_GRAPH_NS::Window>& window, VkInstance instance);
        static const char* GetSurfaceExtensionName();
    };
}

#endif //SR_ENGINE_X11SURFACEINIT_H
