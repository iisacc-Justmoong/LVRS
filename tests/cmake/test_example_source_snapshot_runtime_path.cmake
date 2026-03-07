cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_SOURCE_DIR OR LVRS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_SOURCE_DIR is required.")
endif()

include("${LVRS_SOURCE_DIR}/cmake/LVRSHelpers.cmake")

_lvrs_internal_example_source_snapshot_runtime_rpath("macos" _lvrs_macos_rpath)
if(NOT _lvrs_macos_rpath STREQUAL "@loader_path/../../../../platforms/macos/lib")
    message(FATAL_ERROR
        "Expected macOS example rpath '@loader_path/../../../../platforms/macos/lib', got '${_lvrs_macos_rpath}'.")
endif()

_lvrs_internal_example_source_snapshot_runtime_rpath("linux" _lvrs_linux_rpath)
if(NOT _lvrs_linux_rpath STREQUAL "$ORIGIN/../../../../platforms/linux/lib")
    message(FATAL_ERROR
        "Expected Linux example rpath '$ORIGIN/../../../../platforms/linux/lib', got '${_lvrs_linux_rpath}'.")
endif()

_lvrs_internal_example_source_snapshot_runtime_rpath("ios" _lvrs_ios_rpath)
if(NOT _lvrs_ios_rpath STREQUAL "")
    message(FATAL_ERROR
        "Expected empty iOS example rpath, got '${_lvrs_ios_rpath}'.")
endif()

_lvrs_internal_example_source_snapshot_runtime_rpath("windows" _lvrs_windows_rpath)
if(NOT _lvrs_windows_rpath STREQUAL "")
    message(FATAL_ERROR
        "Expected empty Windows example rpath, got '${_lvrs_windows_rpath}'.")
endif()
