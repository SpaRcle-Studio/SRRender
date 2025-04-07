message(STATUS "Building Skia library.")

set(SR_SKIA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/skia")
set(SR_BUILD_FILE "${SR_SKIA_SOURCE_DIR}/srengine.build")

if (EXISTS "${SR_BUILD_FILE}")
    message(STATUS "Build file already exists. Skipping Skia build.")
    return()
endif()

if (UNIX AND NOT ANDROID_NDK)
    set(SR_SKIA_GN_EXECUTABLE "${SR_SKIA_SOURCE_DIR}/bin/gn")
elseif (WIN32 OR ANDROID_NDK)
    set(SR_SKIA_GN_EXECUTABLE "${SR_SKIA_SOURCE_DIR}/bin/gn.exe")
else()
    message(FATAL_ERROR "Unsupported platform for Skia build.")
endif()

if (NOT EXISTS "${SR_SKIA_GN_EXECUTABLE}")
    message(STATUS "Downloading GN for Skia...")
    execute_process(
            COMMAND ${SR_PYTHON_EXECUTABLE} bin/fetch-gn
            RESULT_VARIABLE result
            OUTPUT_VARIABLE output
            ERROR_VARIABLE error_output
            WORKING_DIRECTORY ${SR_SKIA_SOURCE_DIR}
    )

    if (${result} EQUAL "0")
        message(STATUS "GN downloaded successfully.")
    else()
        message(FATAL_ERROR "Failed to download GN: ${result}")
    endif()
else()
    message(STATUS "GN already exists. Skipping download.")
endif()

set(NINJA_PATH "${SR_SKIA_SOURCE_DIR}/third_party/ninja/ninja")
if (NOT EXISTS "${NINJA_PATH}")
    message(STATUS "Downloading Ninja for Skia...")
    execute_process(
            COMMAND ${SR_PYTHON_EXECUTABLE} bin/fetch-ninja
            RESULT_VARIABLE result
            OUTPUT_VARIABLE output
            ERROR_VARIABLE error_output
            WORKING_DIRECTORY ${SR_SKIA_SOURCE_DIR}
    )

    if (${result} EQUAL "0")
        message(STATUS "Ninja downloaded successfully.")
    else()
        message(FATAL_ERROR "Failed to download Ninja: ${result}")
    endif()
else()
    message(STATUS "Ninja already exists. Skipping download.")
endif()

message(STATUS "Installing Skia dependencies...")
execute_process(
        COMMAND ${SR_PYTHON_EXECUTABLE} tools/git-sync-deps
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error_output
        WORKING_DIRECTORY ${SR_SKIA_SOURCE_DIR}
)

if (result EQUAL "0")
    message(STATUS "Skia dependencies installed successfully.")
else()
    message(FATAL_ERROR "Failed to install Skia dependencies: ${result}")
endif()

message(STATUS "Generating Skia build files...")
set(SR_SKIA_BUILD_DIR "${SR_SKIA_SOURCE_DIR}/out/srengine")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(__IS_DEBUG "true")
elseif (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set(__IS_DEBUG "false")
else()
    message(FATAL_ERROR "Unsupported build type: ${CMAKE_BUILD_TYPE}. Supported types are Debug and Release.")
endif()

# Generic draft build command for Windows and Linux
set(SR_SKIA_GN_COMMAND "gen" ${SR_SKIA_BUILD_DIR} "--args=is_official_build=false is_component_build=false skia_use_vulkan=true skia_use_gl=false is_debug=${__IS_DEBUG}")

message(STATUS "Skia GN command: ${SR_SKIA_GN_COMMAND}")

execute_process(
        COMMAND ${SR_SKIA_GN_EXECUTABLE} ${SR_SKIA_GN_COMMAND}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error_output
        WORKING_DIRECTORY ${SR_SKIA_SOURCE_DIR}
)

if (result EQUAL "0")
    message(STATUS "Skia build files generated successfully.")
else()
    message(FATAL_ERROR "Failed to generate Skia build files: ${output}")
endif()


message(STATUS "Compiling Skia...")

include(ProcessorCount)
ProcessorCount(CPU_CORES)
if(NOT CPU_CORES EQUAL 0)
    message(STATUS "CPU cores: ${CPU_CORES}")
else()
    message(WARNING "Failed to detect number of CPU cores")
endif()

#find_program(NINJA_EXECUTABLE ninja)
#if(NINJA_EXECUTABLE)
#    message(STATUS "Found Ninja: ${NINJA_EXECUTABLE}")
#else()
#    message(FATAL_ERROR "Ninja not found!")
#endif()

execute_process(COMMAND
    ${NINJA_PATH} -C "${SR_SKIA_BUILD_DIR}" -j ${CPU_CORES}
    RESULT_VARIABLE ninja_result
)

if (ninja_result)
    message(FATAL_ERROR "Building Skia failed: ${ninja_result}")
else()
    message(STATUS "Skia built successfully.")
    file(WRITE "${SR_BUILD_FILE}" "")
endif()
