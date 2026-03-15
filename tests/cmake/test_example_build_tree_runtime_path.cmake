cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_SOURCE_DIR OR LVRS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_SOURCE_DIR is required.")
endif()

include("${LVRS_SOURCE_DIR}/cmake/LVRSHelpers.cmake")

_lvrs_internal_example_build_tree_runtime_rpath("macos" _lvrs_macos_rpath)
if(NOT _lvrs_macos_rpath STREQUAL "@loader_path/../../../")
    message(FATAL_ERROR
        "Expected macOS build-tree example rpath '@loader_path/../../../', got '${_lvrs_macos_rpath}'.")
endif()

_lvrs_internal_example_build_tree_runtime_rpath("linux" _lvrs_linux_rpath)
if(NOT _lvrs_linux_rpath STREQUAL "")
    message(FATAL_ERROR
        "Expected empty Linux build-tree example rpath, got '${_lvrs_linux_rpath}'.")
endif()

_lvrs_internal_example_build_tree_runtime_rpath("ios" _lvrs_ios_rpath)
if(NOT _lvrs_ios_rpath STREQUAL "")
    message(FATAL_ERROR
        "Expected empty iOS build-tree example rpath, got '${_lvrs_ios_rpath}'.")
endif()

_lvrs_internal_example_build_tree_runtime_rpath("windows" _lvrs_windows_rpath)
if(NOT _lvrs_windows_rpath STREQUAL "")
    message(FATAL_ERROR
        "Expected empty Windows build-tree example rpath, got '${_lvrs_windows_rpath}'.")
endif()
