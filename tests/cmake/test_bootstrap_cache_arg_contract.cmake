cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_SOURCE_DIR OR LVRS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_SOURCE_DIR is required.")
endif()

include("${LVRS_SOURCE_DIR}/cmake/LVRSBootstrapCacheArgs.cmake")

foreach(_lvrs_action IN ITEMS
    "LVRSBootstrapAction.cmake"
    "LVRSBootstrapFrameworkAction.cmake"
)
    file(READ "${LVRS_SOURCE_DIR}/cmake/${_lvrs_action}" _lvrs_action_content)
    string(FIND
        "${_lvrs_action_content}"
        "include(\"\${CMAKE_CURRENT_LIST_DIR}/LVRSBootstrapCacheArgs.cmake\")"
        _lvrs_helper_include_index
    )
    if(_lvrs_helper_include_index EQUAL -1)
        message(FATAL_ERROR "${_lvrs_action} must use LVRSBootstrapCacheArgs.cmake.")
    endif()

    string(FIND
        "${_lvrs_action_content}"
        "_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd \"CMAKE_OSX_DEPLOYMENT_TARGET\" \"\${LVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET}\")"
        _lvrs_osx_deployment_target_index
    )
    if(_lvrs_osx_deployment_target_index EQUAL -1)
        message(FATAL_ERROR
            "${_lvrs_action} must propagate LVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET.")
    endif()
endforeach()

file(READ "${LVRS_SOURCE_DIR}/cmake/LVRSHelpers.cmake" _lvrs_helpers_content)
string(REGEX MATCHALL
    "-DLVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET=\\$\\{_lvrs_osx_deployment_target\\}"
    _lvrs_osx_deployment_target_forwarding
    "${_lvrs_helpers_content}"
)
list(LENGTH _lvrs_osx_deployment_target_forwarding _lvrs_osx_deployment_target_forwarding_count)
if(NOT _lvrs_osx_deployment_target_forwarding_count EQUAL 2)
    message(FATAL_ERROR
        "Expected both framework and application bootstrap targets to forward the resolved "
        "Apple deployment target, found ${_lvrs_osx_deployment_target_forwarding_count} occurrences.")
endif()

file(READ
    "${LVRS_SOURCE_DIR}/cmake/LVRSBootstrapFrameworkAction.cmake"
    _lvrs_framework_action_content
)
foreach(_lvrs_framework_required IN ITEMS
    "if(NOT DEFINED LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS)"
    "if(NOT DEFINED LVRS_BOOTSTRAP_LVRS_ENABLE_IPO)"
    "_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd \"LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS\" \"\${LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS}\")"
    "_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd \"LVRS_ENABLE_IPO\" \"\${LVRS_BOOTSTRAP_LVRS_ENABLE_IPO}\")"
)
    string(FIND
        "${_lvrs_framework_action_content}"
        "${_lvrs_framework_required}"
        _lvrs_framework_required_index
    )
    if(_lvrs_framework_required_index EQUAL -1)
        message(FATAL_ERROR
            "LVRSBootstrapFrameworkAction.cmake must contain '${_lvrs_framework_required}'.")
    endif()
endforeach()

set(_lvrs_command cmake)
_lvrs_bootstrap_append_cache_arg(_lvrs_command "BOOL_OFF" "OFF")
_lvrs_bootstrap_append_cache_arg(_lvrs_command "BOOL_ON" "ON")
_lvrs_bootstrap_append_cache_arg(_lvrs_command "EMPTY" "")
_lvrs_bootstrap_append_cache_arg(_lvrs_command "LIST" "one;two")

list(JOIN _lvrs_command "|" _lvrs_command_text)
foreach(_lvrs_expected IN ITEMS
    "-DBOOL_OFF=OFF"
    "-DBOOL_ON=ON"
    "-DLIST=one;two"
)
    string(FIND "${_lvrs_command_text}" "${_lvrs_expected}" _lvrs_expected_index)
    if(_lvrs_expected_index EQUAL -1)
        message(FATAL_ERROR
            "Expected bootstrap cache argument '${_lvrs_expected}', got '${_lvrs_command_text}'.")
    endif()
endforeach()

list(LENGTH _lvrs_command _lvrs_command_length)
if(NOT _lvrs_command_length EQUAL 4)
    message(FATAL_ERROR
        "Semicolon cache values must remain one command argument: ${_lvrs_command_text}")
endif()

string(FIND "${_lvrs_command_text}" "-DEMPTY=" _lvrs_empty_index)
if(NOT _lvrs_empty_index EQUAL -1)
    message(FATAL_ERROR "Empty bootstrap cache values must be omitted: ${_lvrs_command_text}")
endif()
