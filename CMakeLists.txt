cmake_minimum_required(VERSION 3.16)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT MSVC AND NOT ANDROID_NDK)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=c++20")
endif()

project(Graphics)

SR_COMMON_GENERATE_BUILD_SOURCES(${PROJECT_SOURCE_DIR})
SR_COMMON_GET_SOURCES(Graphics)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(BUILD_SHARED_LIBS OFF)

set(FT_CONFIG_OPTION_USE_ZLIdB OFF)
set(FT_DISABLE_HARFBUZZ ON)

option(SR_GRAPHICS_DLL_EXPORTS "" ON)
option(SR_RENDER_USE_GLSL_LANG_LIB "" ON)

option(SR_RENDER_GLFW "" OFF)
option(SR_RENDER_USE_YOGA "" ON)

SRUseAssimpIfPossible(Graphics)

if (SR_EMSCRIPTEN)
    set(SR_RENDER_USE_VULKAN OFF)
else()
    set(SR_RENDER_USE_VULKAN ON)
endif()

if (SR_RENDER_USE_VULKAN)
    message("$ENV{VULKAN_SDK}")
    find_package(Vulkan REQUIRED)
endif()

if (NOT ANDROID_NDK AND UNIX AND NOT APPLE AND NOT SR_EMSCRIPTEN)
    set(SR_RENDER_GLFW ON)
endif()

if (SR_RENDER_GLFW)
    set(SR_RENDER_GLFW ON)

    if (SR_HOST_SYSTEM_UNIX)
        #set(GLFW_BUILD_WAYLAND ON)
        set(GLFW_BUILD_X11 ON)
    endif()

    add_subdirectory(libs/glfw)
    include_directories(libs/glfw/include)
endif()

if (SR_RENDER_USE_VULKAN)
    set(EVO_VULKAN_USE_OWN_GLFW OFF)
    add_subdirectory(libs/EvoVulkan/Core)
endif()

if (ANDROID_NDK)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-function -Wunused-function -Wno-comment -Wunused-variable")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unused-function -Wunused-function -Wno-comment -Wunused-variable")
endif()

if (NOT SR_EMSCRIPTEN)
    include(freetype.cmake)
endif()

if (SR_RENDER_GLFW)
    add_compile_definitions(SR_RENDER_GLFW)
endif()

if (NOT SR_EMSCRIPTEN)
    add_subdirectory(libs/cmp_core)
endif()

if (SR_GRAPHICS_STATIC_LIBRARY)
    add_library(Graphics STATIC ${Graphics_SOURCES})

    if (CYGWIN AND SR_RENDER_USE_VULKAN)
        target_link_libraries(Graphics "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
    else()

    endif()
else()
    add_library(Graphics SHARED ${Graphics_SOURCES})
    add_library(Graphics::lib ALIAS Graphics)

    if (MINGW AND SR_RENDER_USE_VULKAN)
        target_link_libraries(Graphics PUBLIC ${Vulkan_LIBRARY} Dwmapi.lib)
    else()
        if (ANDROID_NDK)

        else()
            if (SR_RENDER_USE_VULKAN)
                target_link_libraries(Graphics PUBLIC ${Vulkan_LIBRARY})
            endif()
        endif()
    endif()

    if (UNIX)
        target_compile_options(Graphics PRIVATE -fPIC)
    endif()

    target_link_libraries(Graphics PUBLIC Utils)

    SR_COPY_SHARED_MODULE_TO_EXECUTABLE_FOLDER(Graphics)
    SR_COPY_STATIC_LIBRARY_TO_LIBRARY_FOLDER(Graphics)
endif()

if (SR_GRAPHICS_DLL_EXPORTS)
    message("Graphics will be using DLL exports")
    target_compile_definitions(Graphics PUBLIC SR_GRAPHICS_DLL_EXPORTS)
endif()

if (SR_RENDER_USE_GLSL_LANG_LIB)
    message("Graphics will be using GLSL lang lib")
    target_compile_definitions(Graphics PUBLIC SR_RENDER_USE_GLSL_LANG_LIB)

    if (SR_USE_PCH)
        set(ENABLE_PCH OFF CACHE BOOL "" FORCE)
    endif()

    set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
    set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
    set(ENABLE_RTTI ON CACHE BOOL "" FORCE)

    add_subdirectory(libs/glslang)
    target_link_libraries(Graphics PUBLIC glslang SPIRV)
    target_include_directories(Graphics PUBLIC libs/glslang/glslang/Public)
    target_include_directories(Graphics PUBLIC libs/glslang/SPIRV)
endif()

if (NOT ANDROID_NDK)
    if (UNIX AND NOT APPLE AND NOT SR_EMSCRIPTEN)
        target_include_directories(Graphics PUBLIC "${X11_xcb_keysyms_INCLUDE_PATH}")
        target_link_libraries(Graphics PUBLIC ${X11_xcb_keysyms_LIB})

        target_include_directories(Graphics PUBLIC "${X11_xcb_cursor_INCLUDE_PATH}")
        target_link_libraries(Graphics PUBLIC ${X11_xcb_cursor_LIB} xcb-cursor)

        target_include_directories(Graphics PUBLIC "${X11_xcb_xfixes_INCLUDE_PATH}")
        target_link_libraries(Graphics PUBLIC ${X11_xcb_xfixes_LIB})

        target_include_directories(Graphics PUBLIC "${X11_xcb_INCLUDE_PATH}")
        target_link_libraries(Graphics PUBLIC ${X11_xcb_LIB})

        target_include_directories(Graphics PUBLIC "${X11_xcb_randr_INCLUDE_PATH}")
        target_link_libraries(Graphics PUBLIC ${X11_xcb_randr_LIB})

        target_include_directories(Graphics PUBLIC ${X11_X11_xcb_INCLUDE_PATH})
        target_link_libraries(Graphics PUBLIC ${X11_X11_xcb_LIB})

        target_include_directories(Graphics PUBLIC ${X11_xcb_xkb_INCLUDE_PATH})
        target_link_libraries(Graphics PUBLIC ${X11_xcb_xkb_LIB})

        target_include_directories(Graphics PUBLIC ${X11_xcb_ewmh_INCLUDE_PATH})
        target_link_libraries(Graphics PUBLIC ${X11_xcb_ewmh_LIB} xcb-ewmh xcb-util)
    endif()
endif()

if (SR_RENDER_USE_YOGA)
    add_subdirectory(libs/yoga)
    target_link_libraries(Graphics PUBLIC yogacore)
    target_include_directories(Graphics PUBLIC libs/yoga)
    target_compile_definitions(Graphics PUBLIC SR_RENDER_USE_YOGA)
endif()

if (NOT SR_EMSCRIPTEN)
    target_link_libraries(Graphics PUBLIC CMP_Core)
    target_include_directories(Graphics PUBLIC libs/cmp_core/source)
endif()

if (NOT SR_EMSCRIPTEN)
    target_link_libraries(Graphics PUBLIC freetype)
endif()

if (SR_RENDER_GLFW)
    target_link_libraries(Graphics PUBLIC glfw)
    #target_include_directories(Graphics PUBLIC libs/glfw/include)
endif()

if (SR_RENDER_USE_VULKAN)
    if (EVO_VULKAN_STATIC_LIBRARY)
        target_link_libraries(Graphics PUBLIC EvoVulkan)
    else()
        target_link_libraries(Graphics PUBLIC EvoVulkan::lib)
    endif()
endif()

if (SR_USE_PCH)
    target_precompile_headers(Graphics PRIVATE ${SR_COMMON_PCH_HEADER_PATH})
endif()

target_compile_definitions(Graphics PRIVATE SR_IMGUI_INCLUDE_CHECK)

target_include_directories(Graphics PUBLIC inc)
target_include_directories(Graphics PUBLIC libs/inc)
target_include_directories(Graphics PUBLIC libs/imgui)
#target_include_directories(Graphics PUBLIC libs/imgui-node-editor)
#target_include_directories(Graphics PUBLIC libs/glew/inc)
target_include_directories(Graphics PUBLIC libs)

if (SR_RENDER_USE_VULKAN)
    target_include_directories(Graphics PUBLIC ${Vulkan_INCLUDE_DIRS})
    target_include_directories(Graphics PUBLIC libs/EvoVulkan/Core/inc)
endif()

get_property(SR_UTILS_INCLUDE_DIRECTORIES_CONTENT GLOBAL PROPERTY SR_UTILS_INCLUDE_DIRECTORIES)
target_include_directories(Graphics PUBLIC ${SR_UTILS_INCLUDE_DIRECTORIES_CONTENT})
