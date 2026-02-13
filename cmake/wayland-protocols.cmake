find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

set(WAYLAND_PROTOCOLS_DIR "/usr/share/wayland-protocols")

set(SR_WAYLAND_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/wayland")
file(MAKE_DIRECTORY "${SR_WAYLAND_GEN_DIR}")

message(STATUS "Generating Wayland protocol code in: ${SR_WAYLAND_GEN_DIR}")

function(gen_wayland protocol_xml header_out code_out)
    if(NOT EXISTS "${protocol_xml}")
        message(WARNING "Wayland protocol XML not found: ${protocol_xml}, skipping generation")
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
endfunction()

# xdg-shell
gen_wayland(
    "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml"
    "${SR_WAYLAND_GEN_DIR}/xdg-shell-client-protocol.h"
    "${SR_WAYLAND_GEN_DIR}/xdg-shell-client-protocol.c"
)

# fractional-scale
gen_wayland(
    "${WAYLAND_PROTOCOLS_DIR}/staging/fractional-scale/fractional-scale-v1.xml"
    "${SR_WAYLAND_GEN_DIR}/fractional-scale-v1-client-protocol.h"
    "${SR_WAYLAND_GEN_DIR}/fractional-scale-v1-client-protocol.c"
)

# xdg-decoration
gen_wayland(
    "${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml"
    "${SR_WAYLAND_GEN_DIR}/xdg-decoration-unstable-v1.h"
    "${SR_WAYLAND_GEN_DIR}/xdg-decoration-unstable-v1.c"
)

function(patch_wl_private file)
    if (NOT EXISTS "${file}")
        message(FATAL_ERROR "Wayland generated file not found: ${file}")
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

message(STATUS "Wayland protocol code generation completed.")