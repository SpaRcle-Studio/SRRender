//
// Created by Monika on 11.12.2021.
//

#include <Graphics/GUI/ImGUI.h>

#include <imgui/imgui.cpp>
#include <imgui/imgui_tables.cpp>
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui_widgets.cpp>
#include <imgui/imgui_demo.cpp>

#ifdef IMGUI_HAS_STACK_LAYOUT
    #include <imgui/imgui_stacklayout.cpp>
#endif

#include <imgui/misc/cpp/imgui_stdlib.cpp>

#ifdef SR_WIN32
    #include <imgui/backends/imgui_impl_win32.cpp>
#endif

#ifdef SR_LINUX
    #include <imgui/backends/imgui_impl_glfw.cpp>
#endif

#ifdef SR_USE_GLFW3
    #include <imgui/backends/imgui_impl_glfw.cpp>
#endif

#ifdef SR_USE_OPENGL
    #include <imgui/backends/imgui_impl_opengl3.cpp>
    /// #include "../Depends/imgui/backends/imgui_impl_glut.cpp"
#endif

#ifdef SR_USE_VULKAN
    #include <imgui/backends/imgui_impl_vulkan.cpp>
#endif
