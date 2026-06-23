cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_SOURCE_DIR OR LVRS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_SOURCE_DIR is required.")
endif()

set(_lvrs_bootstrap_action "${LVRS_SOURCE_DIR}/cmake/LVRSBootstrapFrameworkAction.cmake")
if(NOT EXISTS "${_lvrs_bootstrap_action}")
    message(FATAL_ERROR "Expected framework bootstrap action at ${_lvrs_bootstrap_action}.")
endif()

file(READ "${_lvrs_bootstrap_action}" _lvrs_bootstrap_action_content)

foreach(_lvrs_required_snippet IN ITEMS
    "--parallel 1"
    "MAKEFLAGS="
    "MFLAGS="
    "CMAKE_BUILD_PARALLEL_LEVEL="
    "COMMAND \${_lvrs_build_env_cmd} \${_lvrs_build_cmd}"
)
    string(FIND "${_lvrs_bootstrap_action_content}" "${_lvrs_required_snippet}" _lvrs_required_index)
    if(_lvrs_required_index EQUAL -1)
        message(FATAL_ERROR
            "Expected LVRSBootstrapFrameworkAction.cmake to contain '${_lvrs_required_snippet}'.")
    endif()
endforeach()
