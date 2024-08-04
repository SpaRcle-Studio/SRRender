cmake_minimum_required(VERSION 3.16)
project(Graphics)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(BUILD_SHARED_LIBS OFF)

set(FT_CONFIG_OPTION_USE_ZLIdB OFF)
set(FT_DISABLE_HARFBUZZ ON)

option(SR_RENDER_GLFW "" OFF)

message("$ENV{VULKAN_SDK}")

find_package(Vulkan REQUIRED)

if (UNIX AND NOT APPLE)
    set(SR_RENDER_GLFW ON)
endif()

if (SR_RENDER_GLFW)
    set(SR_RENDER_GLFW ON)

    add_subdirectory(libs/glfw)
    include_directories(libs/glfw/include)
endif()

set(EVO_VULKAN_USE_OWN_GLFW OFF)
add_subdirectory(libs/EvoVulkan/Core)

if (ANDROID_NDK)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-function -Wunused-function -Wno-comment -Wunused-variable")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-function -Wunused-function -Wno-comment -Wunused-variable")
endif()

include(freetype.cmake)

if (SR_RENDER_GLFW)
    add_compile_definitions(SR_RENDER_GLFW)
endif()

add_subdirectory(libs/cmp_core)

set(SR_GRAPHICS_SOURCES cxx/Graphics.cxx cxx/ImGUI.cxx cxx/SRSL.cxx cxx/Pass.cxx cxx/GUI.cxx cxx/Animations.cxx)

if (SR_GRAPHICS_STATIC_LIBRARY)
    add_library(Graphics STATIC ${SR_GRAPHICS_SOURCES})

    if (CYGWIN)
        target_link_libraries(Graphics "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
    else()

    endif()
else()
    add_library(Graphics SHARED ${SR_GRAPHICS_SOURCES})
    add_library(Graphics::lib ALIAS Graphics)

    if (MINGW)
        target_link_libraries(Graphics ${Vulkan_LIBRARY} Dwmapi.lib)
    else()
        if (ANDROID_NDK)

        else()
            target_link_libraries(Graphics ${Vulkan_LIBRARY})
        endif()
    endif()
endif()

if (UNIX AND NOT APPLE)
    target_include_directories(Graphics PUBLIC "${X11_xcb_keysyms_INCLUDE_PATH}")
    target_link_libraries(Graphics ${X11_xcb_keysyms_LIB})

    target_include_directories(Graphics PUBLIC "${X11_xcb_cursor_INCLUDE_PATH}")
    target_link_libraries(Graphics ${X11_xcb_cursor_LIB} xcb-cursor)

    target_include_directories(Graphics PUBLIC "${X11_xcb_xfixes_INCLUDE_PATH}")
    target_link_libraries(Graphics ${X11_xcb_xfixes_LIB})

    target_include_directories(Graphics PUBLIC "${X11_xcb_INCLUDE_PATH}")
    target_link_libraries(Graphics ${X11_xcb_LIB})

    target_include_directories(Graphics PUBLIC "${X11_xcb_randr_INCLUDE_PATH}")
    target_link_libraries(Graphics ${X11_xcb_randr_LIB})

    target_include_directories(Graphics PUBLIC ${X11_X11_xcb_INCLUDE_PATH})
    target_link_libraries(Graphics ${X11_X11_xcb_LIB})

    target_include_directories(Graphics PUBLIC ${X11_xcb_xkb_INCLUDE_PATH})
    target_link_libraries(Graphics ${X11_xcb_xkb_LIB})

    target_include_directories(Graphics PUBLIC ${X11_xcb_ewmh_INCLUDE_PATH})
    target_link_libraries(Graphics ${X11_xcb_ewmh_LIB} xcb-ewmh xcb-util)
endif()

target_link_libraries(Graphics CMP_Core freetype)

if (UNIX AND NOT APPLE)
    target_link_libraries(Graphics glfw)
    #target_include_directories(Graphics PUBLIC libs/glfw/include)
endif()

if (EVO_VULKAN_STATIC_LIBRARY)
    target_link_libraries(Graphics EvoVulkan)
else()
    target_link_libraries(Graphics EvoVulkan::lib)
endif()

target_include_directories(Graphics PUBLIC inc)
target_include_directories(Graphics PUBLIC libs/EvoVulkan/Core/inc)
target_include_directories(Graphics PUBLIC libs/cmp_core/source)
target_include_directories(Graphics PUBLIC libs/inc)
target_include_directories(Graphics PUBLIC libs/imgui)
#target_include_directories(Graphics PUBLIC libs/imgui-node-editor)
target_include_directories(Graphics PUBLIC libs/ImGuizmo)
#target_include_directories(Graphics PUBLIC libs/glew/inc)
target_include_directories(Graphics PUBLIC libs)
target_include_directories(Graphics PUBLIC ${Vulkan_INCLUDE_DIRS})

get_property(SR_UTILS_INCLUDE_DIRECTORIES_CONTENT GLOBAL PROPERTY SR_UTILS_INCLUDE_DIRECTORIES)
target_include_directories(Graphics PUBLIC ${SR_UTILS_INCLUDE_DIRECTORIES_CONTENT})
