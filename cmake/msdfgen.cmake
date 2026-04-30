set(MSDFGEN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/libs/msdfgen)
message(STATUS "Including msdfgen from ${MSDFGEN_ROOT}")
set(MSDFGEN_BUILD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

include(${MSDFGEN_ROOT}/cmake/version.cmake)

set(MSDFGEN_CORE_ONLY OFF CACHE BOOL "" FORCE)
set(MSDFGEN_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_SKIA OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_VCPKG OFF CACHE BOOL "" FORCE)
set(MSDFGEN_DISABLE_SVG ON CACHE BOOL "" FORCE)
set(MSDFGEN_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(MSDFGEN_INSTALL ON CACHE BOOL "" FORCE)

#option(MSDFGEN_CORE_ONLY "Only build the core library with no dependencies" OFF)
#option(MSDFGEN_BUILD_STANDALONE "Build the msdfgen standalone executable" ON)
#option(MSDFGEN_USE_VCPKG "Use vcpkg package manager to link project dependencies" ON)
#option(MSDFGEN_USE_OPENMP "Build with OpenMP support for multithreaded code" OFF)
#option(MSDFGEN_USE_CPP11 "Build with C++11 enabled" ON)
#option(MSDFGEN_USE_SKIA "Build with the Skia library" ON)
#option(MSDFGEN_DISABLE_SVG "Disable SVG support" OFF)
#option(MSDFGEN_DISABLE_PNG "Disable PNG support" OFF)
#option(MSDFGEN_INSTALL "Generate installation target" OFF)
#option(MSDFGEN_DYNAMIC_RUNTIME "Link dynamic runtime library instead of static" OFF)
#option(BUILD_SHARED_LIBS "Generate dynamic library files instead of static" OFF)

get_property(MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to Release")
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSDFGEN_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
endif()

if(BUILD_SHARED_LIBS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

file(GLOB_RECURSE MSDFGEN_CORE_SOURCES "${MSDFGEN_ROOT}/core/*.cpp")
file(GLOB_RECURSE MSDFGEN_CORE_HEADERS "${MSDFGEN_ROOT}/core/*.h" "${MSDFGEN_ROOT}/core/*.hpp")
file(GLOB_RECURSE MSDFGEN_EXT_SOURCES "${MSDFGEN_ROOT}/ext/*.cpp" "${MSDFGEN_ROOT}/lib/*.cpp")
file(GLOB_RECURSE MSDFGEN_EXT_HEADERS "${MSDFGEN_ROOT}/ext/*.h" "${MSDFGEN_ROOT}/ext/*.hpp")

# Core library
add_library(msdfgen-core "${MSDFGEN_ROOT}/msdfgen.h" ${MSDFGEN_CORE_HEADERS} ${MSDFGEN_CORE_SOURCES})
add_library(msdfgen::msdfgen-core ALIAS msdfgen-core)
set_target_properties(msdfgen-core PROPERTIES PUBLIC_HEADER "${MSDFGEN_CORE_HEADERS}")
target_compile_definitions(msdfgen-core PUBLIC
        MSDFGEN_VERSION=${MSDFGEN_VERSION}
        MSDFGEN_VERSION_MAJOR=${MSDFGEN_VERSION_MAJOR}
        MSDFGEN_VERSION_MINOR=${MSDFGEN_VERSION_MINOR}
        MSDFGEN_VERSION_REVISION=${MSDFGEN_VERSION_REVISION}
        MSDFGEN_COPYRIGHT_YEAR=${MSDFGEN_COPYRIGHT_YEAR}
)
target_include_directories(msdfgen-core INTERFACE
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/msdfgen>
        $<BUILD_INTERFACE:${MSDFGEN_ROOT}/>
)

if(MSDFGEN_USE_CPP11)
    target_compile_features(msdfgen-core PUBLIC cxx_std_11)
    target_compile_definitions(msdfgen-core PUBLIC MSDFGEN_USE_CPP11)
endif()

if(MSDFGEN_USE_OPENMP)
    # Note: Clang doesn't support OpenMP by default...
    find_package(OpenMP REQUIRED COMPONENTS CXX)
    target_compile_definitions(msdfgen-core PUBLIC MSDFGEN_USE_OPENMP)
    target_link_libraries(msdfgen-core PUBLIC OpenMP::OpenMP_CXX)
endif()

if(BUILD_SHARED_LIBS AND WIN32)
    target_compile_definitions(msdfgen-core PRIVATE "MSDFGEN_PUBLIC=__declspec(dllexport)")
    target_compile_definitions(msdfgen-core INTERFACE "MSDFGEN_PUBLIC=__declspec(dllimport)")
else()
    target_compile_definitions(msdfgen-core PUBLIC MSDFGEN_PUBLIC=)
endif()

# Extensions library
if(NOT MSDFGEN_CORE_ONLY)
    if(NOT c AND NOT TARGET PNG::PNG)
        find_package(PNG REQUIRED)
    endif()

    add_library(msdfgen-ext "${MSDFGEN_ROOT}/msdfgen-ext.h" ${MSDFGEN_EXT_HEADERS} ${MSDFGEN_EXT_SOURCES})
    add_library(msdfgen::msdfgen-ext ALIAS msdfgen-ext)
    set_target_properties(msdfgen-ext PROPERTIES PUBLIC_HEADER "${MSDFGEN_EXT_HEADERS}")
    target_compile_definitions(msdfgen-ext INTERFACE MSDFGEN_EXTENSIONS)
    if(NOT MSDFGEN_DISABLE_SVG)
        target_compile_definitions(msdfgen-ext PUBLIC MSDFGEN_USE_TINYXML2)
        target_link_libraries(msdfgen-ext PRIVATE tinyxml2::tinyxml2)
    else()
        target_compile_definitions(msdfgen-ext PUBLIC MSDFGEN_DISABLE_SVG)
    endif()
    if(NOT MSDFGEN_DISABLE_PNG)
        target_compile_definitions(msdfgen-ext PUBLIC MSDFGEN_USE_LIBPNG)
        target_link_libraries(msdfgen-ext PRIVATE PNG::PNG)
    else()
        target_compile_definitions(msdfgen-ext PUBLIC MSDFGEN_DISABLE_PNG)
    endif()
    if(MSDFGEN_DISABLE_VARIABLE_FONTS)
        target_compile_definitions(msdfgen-ext PUBLIC MSDFGEN_DISABLE_VARIABLE_FONTS)
    endif()
    target_link_libraries(msdfgen-ext PRIVATE msdfgen::msdfgen-core)
    target_include_directories(msdfgen-ext PRIVATE ${MSDFGEN_ROOT}/../freetype/include)
    target_include_directories(msdfgen-ext
            PUBLIC
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/msdfgen>
            $<BUILD_INTERFACE:${MSDFGEN_ROOT}>
            PRIVATE
            ${MSDFGEN_ROOT}/include
    )

    if(BUILD_SHARED_LIBS AND WIN32)
        target_compile_definitions(msdfgen-ext PRIVATE "MSDFGEN_EXT_PUBLIC=__declspec(dllexport)")
        target_compile_definitions(msdfgen-ext INTERFACE "MSDFGEN_EXT_PUBLIC=__declspec(dllimport)")
    else()
        target_compile_definitions(msdfgen-ext PUBLIC MSDFGEN_EXT_PUBLIC=)
    endif()

    add_library(msdfgen-full INTERFACE)
    add_library(msdfgen::msdfgen ALIAS msdfgen-full)
    target_link_libraries(msdfgen-full INTERFACE msdfgen::msdfgen-core msdfgen::msdfgen-ext)
else()
    add_library(msdfgen::msdfgen ALIAS msdfgen-core)
endif()

# Hide ZERO_CHECK and ALL_BUILD targets
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER meta)

# Installation
if(MSDFGEN_INSTALL)
    set(MSDFGEN_CONFIG_PATH "lib/cmake/msdfgen")

    # Generate msdfgen-config.h
    if(BUILD_SHARED_LIBS AND WIN32)
        set(MSDFGEN_PUBLIC_MACRO_VALUE " __declspec(dllimport)")
    else()
        set(MSDFGEN_PUBLIC_MACRO_VALUE "")
    endif()
    set(MSDFGEN_ADDITIONAL_DEFINES "")
    if(MSDFGEN_USE_CPP11)
        set(MSDFGEN_ADDITIONAL_DEFINES "${MSDFGEN_ADDITIONAL_DEFINES}\n#define MSDFGEN_USE_CPP11")
    endif()
    if(MSDFGEN_USE_OPENMP)
        set(MSDFGEN_ADDITIONAL_DEFINES "${MSDFGEN_ADDITIONAL_DEFINES}\n#define MSDFGEN_USE_OPENMP")
    endif()
    if(NOT MSDFGEN_CORE_ONLY)
        set(MSDFGEN_ADDITIONAL_DEFINES "${MSDFGEN_ADDITIONAL_DEFINES}\n#define MSDFGEN_EXTENSIONS")
        if(MSDFGEN_USE_SKIA)
            set(MSDFGEN_ADDITIONAL_DEFINES "${MSDFGEN_ADDITIONAL_DEFINES}\n#define MSDFGEN_USE_SKIA")
        endif()
        if(NOT MSDFGEN_DISABLE_SVG)
            set(MSDFGEN_ADDITIONAL_DEFINES "${MSDFGEN_ADDITIONAL_DEFINES}\n#define MSDFGEN_USE_TINYXML2")
        else()
            set(MSDFGEN_ADDITIONAL_DEFINES "${MSDFGEN_ADDITIONAL_DEFINES}\n#define MSDFGEN_DISABLE_SVG")
        endif()
        if(NOT MSDFGEN_DISABLE_PNG)
            set(MSDFGEN_ADDITIONAL_DEFINES "${MSDFGEN_ADDITIONAL_DEFINES}\n#define MSDFGEN_USE_LIBPNG")
        else()
            set(MSDFGEN_ADDITIONAL_DEFINES "${MSDFGEN_ADDITIONAL_DEFINES}\n#define MSDFGEN_DISABLE_PNG")
        endif()
        if(MSDFGEN_DISABLE_VARIABLE_FONTS)
            set(MSDFGEN_ADDITIONAL_DEFINES "${MSDFGEN_ADDITIONAL_DEFINES}\n#define MSDFGEN_DISABLE_VARIABLE_FONTS")
        endif()
    endif()
    configure_file("${MSDFGEN_ROOT}/cmake/msdfgen-config.h.in" msdfgen-config.h)

    if (NOT MSDFGEN_INSTALL_NO_GLOBAL_INCLUDE)
        write_file("${CMAKE_CURRENT_BINARY_DIR}/msdfgen.h" "\n#pragma once\n\n#include \"msdfgen/msdfgen.h\"")
        write_file("${CMAKE_CURRENT_BINARY_DIR}/msdfgen-ext.h" "\n#pragma once\n\n#include \"msdfgen/msdfgen-ext.h\"")
    endif()

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/msdfgenConfigVersion.cmake"
            VERSION 1.13.0
            COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
            "${MSDFGEN_ROOT}/cmake/msdfgenConfig.cmake.in"
            ${MSDFGEN_CONFIG_PATH}/msdfgenConfig.cmake
            INSTALL_DESTINATION ${MSDFGEN_CONFIG_PATH}
            NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    configure_file(
            "${MSDFGEN_ROOT}/cmake/msdfgenConfig.cmake.in"
            msdfgenConfig.cmake
            @ONLY
    )

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/msdfgen-config.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/msdfgen)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/msdfgen-config.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/msdfgen/msdfgen)
    install(TARGETS msdfgen-core EXPORT msdfgenTargets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/msdfgen/core
    )
    install(FILES "${MSDFGEN_ROOT}/msdfgen.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/msdfgen)
    if (NOT MSDFGEN_INSTALL_NO_GLOBAL_INCLUDE)
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/msdfgen.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif()
    if(MSVC AND BUILD_SHARED_LIBS)
        install(FILES $<TARGET_PDB_FILE:msdfgen-core> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
    endif()

    if(NOT MSDFGEN_CORE_ONLY)
        install(TARGETS msdfgen-ext EXPORT msdfgenTargets
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
                PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/msdfgen/ext
        )
        install(FILES "${MSDFGEN_ROOT}/msdfgen-ext.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/msdfgen)
        if (NOT MSDFGEN_INSTALL_NO_GLOBAL_INCLUDE)
            install(FILES "${CMAKE_CURRENT_BINARY_DIR}/msdfgen-ext.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
        endif()
        if(MSVC AND BUILD_SHARED_LIBS)
            install(FILES $<TARGET_PDB_FILE:msdfgen-ext> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
        endif()
        install(TARGETS msdfgen-full EXPORT msdfgenTargets)
    endif()

    export(EXPORT msdfgenTargets NAMESPACE msdfgen:: FILE "${CMAKE_CURRENT_BINARY_DIR}/msdfgenTargets.cmake")
    install(EXPORT msdfgenTargets FILE msdfgenTargets.cmake NAMESPACE msdfgen:: DESTINATION ${MSDFGEN_CONFIG_PATH})

    if(MSDFGEN_BUILD_STANDALONE)
        install(TARGETS msdfgen EXPORT msdfgenBinaryTargets DESTINATION ${CMAKE_INSTALL_BINDIR})
        if(MSVC)
            install(FILES $<TARGET_PDB_FILE:msdfgen> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
        endif()
        export(EXPORT msdfgenBinaryTargets NAMESPACE msdfgen-standalone:: FILE "${CMAKE_CURRENT_BINARY_DIR}/msdfgenBinaryTargets.cmake")
        install(EXPORT msdfgenBinaryTargets FILE msdfgenBinaryTargets.cmake NAMESPACE msdfgen-standalone:: DESTINATION ${MSDFGEN_CONFIG_PATH})
    endif()

    install(
            FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${MSDFGEN_CONFIG_PATH}/msdfgenConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/msdfgenConfigVersion.cmake"
            DESTINATION ${MSDFGEN_CONFIG_PATH}
    )
endif()
