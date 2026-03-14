cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_SOURCE_DIR OR LVRS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_SOURCE_DIR is required.")
endif()
if(NOT DEFINED LVRS_BINARY_DIR OR LVRS_BINARY_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_BINARY_DIR is required.")
endif()

set(_lvrs_probe_root "${LVRS_BINARY_DIR}/tests/app-bootstrap-skip-probe")
set(_lvrs_probe_source_dir "${_lvrs_probe_root}/src")
set(_lvrs_probe_build_dir "${_lvrs_probe_root}/build")

file(REMOVE_RECURSE "${_lvrs_probe_root}")
file(MAKE_DIRECTORY "${_lvrs_probe_source_dir}")

set(_lvrs_probe_cmakelists [=[
cmake_minimum_required(VERSION 3.21)
project(LVRSBootstrapSkipProbe LANGUAGES CXX)

set(LVRS_RUNTIME_PLATFORMS "ios")
set(LVRS_BOOTSTRAP_QT_PREFIX_IOS "/definitely/missing/qt-prefix")
unset(Qt6_DIR CACHE)
unset(Qt6_DIR)

include("@LVRS_SOURCE_DIR@/cmake/LVRSHelpers.cmake")

add_custom_target(ProbeApp)
lvrs_add_bootstrap_targets(TARGET ProbeApp)
]=])
string(CONFIGURE "${_lvrs_probe_cmakelists}" _lvrs_probe_cmakelists @ONLY)
file(WRITE "${_lvrs_probe_source_dir}/CMakeLists.txt" "${_lvrs_probe_cmakelists}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${_lvrs_probe_source_dir}" -B "${_lvrs_probe_build_dir}"
    RESULT_VARIABLE _lvrs_configure_result
    OUTPUT_VARIABLE _lvrs_configure_stdout
    ERROR_VARIABLE _lvrs_configure_stderr
)

if(NOT _lvrs_configure_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to configure app bootstrap skip probe.\nstdout:\n${_lvrs_configure_stdout}\nstderr:\n${_lvrs_configure_stderr}")
endif()

set(_lvrs_configure_output "${_lvrs_configure_stdout}\n${_lvrs_configure_stderr}")
string(FIND "${_lvrs_configure_output}" "LVRS app bootstrap targets: skipping 'ios' (Qt kit not found)." _lvrs_skip_message_index)
if(_lvrs_skip_message_index LESS 0)
    message(FATAL_ERROR
        "Expected missing Qt kit skip status message during app bootstrap probe configure.\n${_lvrs_configure_output}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${_lvrs_probe_build_dir}" --target help
    RESULT_VARIABLE _lvrs_help_result
    OUTPUT_VARIABLE _lvrs_help_stdout
    ERROR_VARIABLE _lvrs_help_stderr
)

if(NOT _lvrs_help_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to inspect app bootstrap probe targets.\nstdout:\n${_lvrs_help_stdout}\nstderr:\n${_lvrs_help_stderr}")
endif()

set(_lvrs_help_output "${_lvrs_help_stdout}\n${_lvrs_help_stderr}")

string(FIND "${_lvrs_help_output}" "bootstrap_ProbeApp_all" _lvrs_bootstrap_all_index)
if(_lvrs_bootstrap_all_index LESS 0)
    message(FATAL_ERROR
        "Expected aggregate app bootstrap target 'bootstrap_ProbeApp_all' in help output.\n${_lvrs_help_output}")
endif()

foreach(_lvrs_unexpected_target
    IN ITEMS
        "bootstrap_ProbeApp_ios"
        "launch_ProbeApp_ios"
        "export_ProbeApp_xcodeproj"
)
    string(FIND "${_lvrs_help_output}" "${_lvrs_unexpected_target}" _lvrs_unexpected_target_index)
    if(NOT _lvrs_unexpected_target_index LESS 0)
        message(FATAL_ERROR
            "Did not expect target '${_lvrs_unexpected_target}' when iOS Qt kit is unavailable.\n${_lvrs_help_output}")
    endif()
endforeach()
