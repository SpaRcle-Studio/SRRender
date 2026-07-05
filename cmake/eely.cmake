message(STATUS "SRRender: Adding eely library")

set(SR_EELY_ROOT_DIR libs/eely/libs/eely)

set(SR_EELY_SOURCE_FILES
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_base.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_and.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_blend.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_clip.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_param_comparison.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_param.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_random.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_speed.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_state_condition.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_state_machine.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_state_transition.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_state.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_node_sum.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_context.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_and.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_base.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_blend.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_clip.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_param_comparison.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_param.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_random.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_speed.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_pose_base.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_state_condition.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_state_machine.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_state_transition.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_state.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player_node_sum.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_player.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph_uncooked.h
    ${SR_EELY_ROOT_DIR}/include/eely/anim_graph/anim_graph.h
    ${SR_EELY_ROOT_DIR}/include/eely/base/assert.h
    ${SR_EELY_ROOT_DIR}/include/eely/base/base_utils.h
    ${SR_EELY_ROOT_DIR}/include/eely/base/bit_reader.h
    ${SR_EELY_ROOT_DIR}/include/eely/base/bit_writer.h
    ${SR_EELY_ROOT_DIR}/include/eely/base/graph.h
    ${SR_EELY_ROOT_DIR}/include/eely/base/string_id.h
    ${SR_EELY_ROOT_DIR}/include/eely/base/time_utils.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_compression_scheme.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_cooking_none_fixed.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_cursor.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_impl_acl.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_impl_base.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_impl_fixed.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_impl_none.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_player_acl.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_player_base.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_player_fixed.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_player_none.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_uncooked.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip_utils.h
    ${SR_EELY_ROOT_DIR}/include/eely/clip/clip.h
    ${SR_EELY_ROOT_DIR}/include/eely/job/job_add.h
    ${SR_EELY_ROOT_DIR}/include/eely/job/job_base.h
    ${SR_EELY_ROOT_DIR}/include/eely/job/job_blend.h
    ${SR_EELY_ROOT_DIR}/include/eely/job/job_clip.h
    ${SR_EELY_ROOT_DIR}/include/eely/job/job_queue.h
    ${SR_EELY_ROOT_DIR}/include/eely/job/job_restore.h
    ${SR_EELY_ROOT_DIR}/include/eely/job/job_save.h
    ${SR_EELY_ROOT_DIR}/include/eely/math/ellipse.h
    ${SR_EELY_ROOT_DIR}/include/eely/math/elliptical_cone.h
    ${SR_EELY_ROOT_DIR}/include/eely/math/float2.h
    ${SR_EELY_ROOT_DIR}/include/eely/math/float3.h
    ${SR_EELY_ROOT_DIR}/include/eely/math/float4.h
    ${SR_EELY_ROOT_DIR}/include/eely/math/math_utils.h
    ${SR_EELY_ROOT_DIR}/include/eely/math/quantization.h
    ${SR_EELY_ROOT_DIR}/include/eely/math/quaternion.h
    ${SR_EELY_ROOT_DIR}/include/eely/math/transform.h
    ${SR_EELY_ROOT_DIR}/include/eely/project/axis_system.h
    ${SR_EELY_ROOT_DIR}/include/eely/project/measurement_unit.h
    ${SR_EELY_ROOT_DIR}/include/eely/project/project_uncooked.h
    ${SR_EELY_ROOT_DIR}/include/eely/project/project.h
    ${SR_EELY_ROOT_DIR}/include/eely/project/resource_base.h
    ${SR_EELY_ROOT_DIR}/include/eely/project/resource_uncooked.h
    ${SR_EELY_ROOT_DIR}/include/eely/project/resource.h
    ${SR_EELY_ROOT_DIR}/include/eely/skeleton/presets.h
    ${SR_EELY_ROOT_DIR}/include/eely/skeleton/skeleton_pose_pool.h
    ${SR_EELY_ROOT_DIR}/include/eely/skeleton/skeleton_pose.h
    ${SR_EELY_ROOT_DIR}/include/eely/skeleton/skeleton_uncooked.h
    ${SR_EELY_ROOT_DIR}/include/eely/skeleton/skeleton_utils.h
    ${SR_EELY_ROOT_DIR}/include/eely/skeleton/skeleton.h
    ${SR_EELY_ROOT_DIR}/include/eely/skeleton_mask/skeleton_mask_uncooked.h
    ${SR_EELY_ROOT_DIR}/include/eely/skeleton_mask/skeleton_mask.h
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_and.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_base.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_blend.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_clip.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_param_comparison.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_param.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_random.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_speed.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_state_condition.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_state_machine.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_state_transition.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_state.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_node_sum.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_and.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_base.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_blend.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_clip.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_param_comparison.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_param.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_random.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_speed.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_pose_base.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_state_condition.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_state_machine.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_state_transition.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_state.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player_node_sum.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_player.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph_uncooked.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/anim_graph/anim_graph.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/base/base_utils.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/base/bit_reader.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/base/bit_writer.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/base/string_id.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/clip/clip_cooking_none_fixed.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/clip/clip_cursor.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/clip/clip_impl_acl.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/clip/clip_impl_fixed.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/clip/clip_impl_none.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/clip/clip_player_acl.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/clip/clip_player_fixed.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/clip/clip_player_none.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/clip/clip_uncooked.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/clip/clip_utils.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/clip/clip.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/job/job_base.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/job/job_queue.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/math/ellipse.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/math/elliptical_cone.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/math/float2.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/math/float3.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/math/quantization.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/math/quaternion.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/math/transform.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/params/params.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/project/project_uncooked.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/project/project.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/project/resource_base.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/project/resource_uncooked.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/project/resource.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/skeleton/presets.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/skeleton/skeleton_pose_pool.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/skeleton/skeleton_pose.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/skeleton/skeleton_uncooked.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/skeleton/skeleton.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/skeleton_mask/skeleton_mask_uncooked.cpp
    ${SR_EELY_ROOT_DIR}/src/eely/skeleton_mask/skeleton_mask.cpp)

set(SR_EELY_INCLUDE_DIRS ${SR_EELY_ROOT_DIR}/include)

add_subdirectory(libs/eely/external/gsl)
add_subdirectory(libs/eely/external/acl)

add_library(eely STATIC ${SR_EELY_SOURCE_FILES})
target_include_directories(eely PUBLIC ${SR_EELY_INCLUDE_DIRS})
target_include_directories(eely PUBLIC libs/eely/external/gsl/include)
target_include_directories(eely PUBLIC libs/eely/external/acl/include)

if (UNIX)
    target_compile_options(eely PRIVATE -fPIC)
endif()

if (WIN32)
    target_compile_definitions(eely PUBLIC EELY_PLATFORM_WIN64)
endif()

if (MSVC AND NOT SR_COMMON_USE_CLANG_EMULATION)
    target_compile_options(eely PUBLIC
        /W4 # treat all warnings as errors
        /WX # treat all warnings as errors
        /wd4067 # unexpected tokens following preprocessor directive - expected a newline
        /wd4505 # unreferenced local function has been removed
    )
endif()

target_include_directories(Graphics PUBLIC ${SR_EELY_INCLUDE_DIRS})
target_link_libraries(Graphics PUBLIC eely)