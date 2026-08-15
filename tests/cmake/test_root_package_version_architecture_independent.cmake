foreach(_lvrs_required_var IN ITEMS LVRS_SOURCE_DIR LVRS_BINARY_DIR)
    if(NOT DEFINED ${_lvrs_required_var} OR "${${_lvrs_required_var}}" STREQUAL "")
        message(FATAL_ERROR "${_lvrs_required_var} is required")
    endif()
endforeach()

set(_lvrs_root_version "${LVRS_BINARY_DIR}/LVRSConfigVersionRoot.cmake")
set(_lvrs_platform_version "${LVRS_BINARY_DIR}/LVRSConfigVersion.cmake")

foreach(_lvrs_version_file IN ITEMS "${_lvrs_root_version}" "${_lvrs_platform_version}")
    if(NOT EXISTS "${_lvrs_version_file}")
        message(FATAL_ERROR "Expected generated package version file: ${_lvrs_version_file}")
    endif()
endforeach()

file(READ "${_lvrs_root_version}" _lvrs_root_version_content)
if(NOT _lvrs_root_version_content MATCHES "set\\(PACKAGE_VERSION")
    message(FATAL_ERROR
        "The root package dispatcher version must be a standalone version file")
endif()
if(_lvrs_root_version_content MATCHES "CMAKE_SIZEOF_VOID_P")
    message(FATAL_ERROR
        "The root package dispatcher version must be architecture independent so it can select WASM from a 64-bit host install")
endif()

file(READ "${_lvrs_platform_version}" _lvrs_platform_version_content)
if(NOT _lvrs_platform_version_content MATCHES "CMAKE_SIZEOF_VOID_P")
    message(FATAL_ERROR
        "Platform package versions must retain their binary architecture compatibility check")
endif()

set(_lvrs_bootstrap_action "${LVRS_SOURCE_DIR}/cmake/LVRSBootstrapFrameworkAction.cmake")
file(READ "${_lvrs_bootstrap_action}" _lvrs_bootstrap_action_content)
if(NOT _lvrs_bootstrap_action_content MATCHES
   "LVRS_BOOTSTRAP_INSTALL_PREFIX}/LVRSConfigVersion\\.cmake")
    message(FATAL_ERROR
        "Framework bootstrap must publish the standalone root version file instead of a platform version file")
endif()
