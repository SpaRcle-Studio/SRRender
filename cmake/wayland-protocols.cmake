find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

set(WAYLAND_PROTOCOLS_DIR "/usr/share/wayland-protocols")

set(SR_WAYLAND_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/wayland")
file(MAKE_DIRECTORY "${SR_WAYLAND_GEN_DIR}")

message(STATUS "Generating Wayland protocol code in: ${SR_WAYLAND_GEN_DIR}")

function(gen_wayland protocol_xml header_out code_out result_var)
    if(NOT EXISTS "${protocol_xml}")
        message(WARNING "Wayland protocol XML not found: ${protocol_xml}, skipping generation")
        set(${result_var} FALSE PARENT_SCOPE)
        return()
    endif()

    message(STATUS "Generating Wayland code for protocol: ${protocol_xml}")

    execute_process(
            COMMAND ${WAYLAND_SCANNER} client-header
            "${protocol_xml}"
            "${header_out}"
            RESULT_VARIABLE res1
            ERROR_VARIABLE err1
    )
    if (NOT res1 EQUAL 0)
        message(FATAL_ERROR "wayland-scanner client-header failed:\n${err1}")
    endif()

    execute_process(
            COMMAND ${WAYLAND_SCANNER} private-code
            "${protocol_xml}"
            "${code_out}"
            RESULT_VARIABLE res2
            ERROR_VARIABLE err2
    )
    if (NOT res2 EQUAL 0)
        message(FATAL_ERROR "wayland-scanner private-code failed:\n${err2}")
    endif()

    set(${result_var} TRUE PARENT_SCOPE)
endfunction()

# xdg-shell
gen_wayland(
    "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml"
    "${SR_WAYLAND_GEN_DIR}/xdg-shell-client-protocol.h"
    "${SR_WAYLAND_GEN_DIR}/xdg-shell-client-protocol.c"
    is_xdg_shell_generated
)

# fractional-scale
gen_wayland(
    "${WAYLAND_PROTOCOLS_DIR}/staging/fractional-scale/fractional-scale-v1.xml"
    "${SR_WAYLAND_GEN_DIR}/fractional-scale-v1-client-protocol.h"
    "${SR_WAYLAND_GEN_DIR}/fractional-scale-v1-client-protocol.c"
    is_fractional_scale_generated
)

# xdg-decoration
gen_wayland(
    "${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml"
    "${SR_WAYLAND_GEN_DIR}/xdg-decoration-unstable-v1.h"
    "${SR_WAYLAND_GEN_DIR}/xdg-decoration-unstable-v1.c"
    is_xdg_decoration_generated
)

function(patch_wl_private file)
    if (NOT EXISTS "${file}")
        message(WARNING "Wayland generated file not found: ${file}")
        return()
    endif()

    file(READ "${file}" _content)

    string(REPLACE
            "WL_PRIVATE const struct wl_interface xdg_wm_base_interface"
            "const struct wl_interface xdg_wm_base_interface"
            _content
            "${_content}"
    )

    string(REPLACE
            "WL_PRIVATE const struct wl_interface wp_fractional_scale_manager_v1_interface"
            "const struct wl_interface wp_fractional_scale_manager_v1_interface"
            _content
            "${_content}"
    )

    file(WRITE "${file}" "${_content}")

    message(STATUS "Patched WL_PRIVATE in ${file}")
endfunction()

patch_wl_private("${SR_WAYLAND_GEN_DIR}/xdg-shell-client-protocol.c")
patch_wl_private("${SR_WAYLAND_GEN_DIR}/xdg-decoration-unstable-v1.c")
patch_wl_private("${SR_WAYLAND_GEN_DIR}/fractional-scale-v1-client-protocol.c")

if (is_xdg_shell_generated)
    set(SR_WAYLAND_HAS_XDG_SHELL ON CACHE BOOL "" FORCE)
else()
    set(SR_WAYLAND_HAS_XDG_SHELL OFF CACHE BOOL "" FORCE)
endif()

if (is_fractional_scale_generated)
    set(SR_WAYLAND_HAS_FRACTIONAL_SCALE ON CACHE BOOL "" FORCE)
else()
    set(SR_WAYLAND_HAS_FRACTIONAL_SCALE OFF CACHE BOOL "" FORCE)
endif()

if (is_xdg_decoration_generated)
    set(SR_WAYLAND_HAS_XDG_DECORATION ON CACHE BOOL "" FORCE)
else()
    set(SR_WAYLAND_HAS_XDG_DECORATION OFF CACHE BOOL "" FORCE)
endif()

message(STATUS "Wayland protocol code generation completed.")