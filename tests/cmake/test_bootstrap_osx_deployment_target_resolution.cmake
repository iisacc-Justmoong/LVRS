cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_SOURCE_DIR OR LVRS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_SOURCE_DIR is required.")
endif()

include("${LVRS_SOURCE_DIR}/cmake/LVRSHelpers.cmake")

set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
_lvrs_internal_bootstrap_osx_deployment_target_for_platform(
    "macos"
    _lvrs_macos_inherited_target
)
if(NOT _lvrs_macos_inherited_target STREQUAL "12.0")
    message(FATAL_ERROR
        "Expected the macOS bootstrap to inherit CMAKE_OSX_DEPLOYMENT_TARGET=12.0, "
        "got '${_lvrs_macos_inherited_target}'.")
endif()

_lvrs_internal_bootstrap_osx_deployment_target_for_platform(
    "ios"
    _lvrs_ios_unset_target
)
if(NOT _lvrs_ios_unset_target STREQUAL "")
    message(FATAL_ERROR
        "The iOS bootstrap must not inherit a host macOS deployment target, "
        "got '${_lvrs_ios_unset_target}'.")
endif()

set(LVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET "13.0")
_lvrs_internal_bootstrap_osx_deployment_target_for_platform(
    "macos"
    _lvrs_macos_generic_target
)
if(NOT _lvrs_macos_generic_target STREQUAL "13.0")
    message(FATAL_ERROR
        "Expected the generic bootstrap deployment target override to win, "
        "got '${_lvrs_macos_generic_target}'.")
endif()

set(LVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET_IOS "16.0")
_lvrs_internal_bootstrap_osx_deployment_target_for_platform(
    "ios"
    _lvrs_ios_specific_target
)
if(NOT _lvrs_ios_specific_target STREQUAL "16.0")
    message(FATAL_ERROR
        "Expected the per-platform iOS deployment target override to win, "
        "got '${_lvrs_ios_specific_target}'.")
endif()
