cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_SOURCE_DIR OR LVRS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_SOURCE_DIR is required.")
endif()
if(NOT DEFINED LVRS_BINARY_DIR OR LVRS_BINARY_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_BINARY_DIR is required.")
endif()

include("${LVRS_SOURCE_DIR}/cmake/LVRSHelpers.cmake")

set(_lvrs_fake_install_root "${LVRS_BINARY_DIR}/tests/fake-lvrs-install")
set(_lvrs_root_config "${_lvrs_fake_install_root}/LVRSConfig.cmake")
set(_lvrs_host_dir "${_lvrs_fake_install_root}/platforms/macos/lib/cmake/LVRS")
set(_lvrs_ios_dir "${_lvrs_fake_install_root}/platforms/ios/lib/cmake/LVRS")

file(MAKE_DIRECTORY "${_lvrs_host_dir}")
file(MAKE_DIRECTORY "${_lvrs_ios_dir}")
file(WRITE "${_lvrs_root_config}" "# fake root config\n")
file(WRITE "${_lvrs_host_dir}/LVRSConfig.cmake" "# fake macOS config\n")
file(WRITE "${_lvrs_ios_dir}/LVRSConfig.cmake" "# fake iOS config\n")

_lvrs_internal_resolve_install_root_from_lvrs_hint("${_lvrs_fake_install_root}" _lvrs_install_root_from_root)
if(NOT _lvrs_install_root_from_root STREQUAL "${_lvrs_fake_install_root}")
    message(FATAL_ERROR
        "Expected install root '${_lvrs_fake_install_root}' from root hint, got '${_lvrs_install_root_from_root}'.")
endif()

_lvrs_internal_resolve_install_root_from_lvrs_hint("${_lvrs_host_dir}" _lvrs_install_root_from_host_dir)
if(NOT _lvrs_install_root_from_host_dir STREQUAL "${_lvrs_fake_install_root}")
    message(FATAL_ERROR
        "Expected install root '${_lvrs_fake_install_root}' from host package hint, got '${_lvrs_install_root_from_host_dir}'.")
endif()

_lvrs_internal_resolve_bootstrap_lvrs_dir("${_lvrs_fake_install_root}" "ios" _lvrs_resolved_from_root)
if(NOT _lvrs_resolved_from_root STREQUAL "${_lvrs_ios_dir}")
    message(FATAL_ERROR
        "Expected iOS LVRS_DIR '${_lvrs_ios_dir}' from install root hint, got '${_lvrs_resolved_from_root}'.")
endif()

_lvrs_internal_resolve_bootstrap_lvrs_dir("${_lvrs_host_dir}" "ios" _lvrs_resolved_from_host_dir)
if(NOT _lvrs_resolved_from_host_dir STREQUAL "${_lvrs_ios_dir}")
    message(FATAL_ERROR
        "Expected iOS LVRS_DIR '${_lvrs_ios_dir}' from host package hint, got '${_lvrs_resolved_from_host_dir}'.")
endif()

set(_lvrs_custom_hint "${LVRS_BINARY_DIR}/tests/custom-lvrs-dir")
_lvrs_internal_resolve_bootstrap_lvrs_dir("${_lvrs_custom_hint}" "ios" _lvrs_resolved_custom)
if(NOT _lvrs_resolved_custom STREQUAL "${_lvrs_custom_hint}")
    message(FATAL_ERROR
        "Expected unresolved custom hint to remain unchanged, got '${_lvrs_resolved_custom}'.")
endif()
