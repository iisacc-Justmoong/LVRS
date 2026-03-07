cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_SOURCE_DIR OR LVRS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_SOURCE_DIR is required.")
endif()

include("${LVRS_SOURCE_DIR}/cmake/LVRSHelpers.cmake")

set(LVRS_EXAMPLE_RUNTIME_OUTPUT_DIR "")
_lvrs_internal_example_runtime_output_dir(_lvrs_default_output_dir)
if(NOT _lvrs_default_output_dir STREQUAL "${CMAKE_CURRENT_BINARY_DIR}/bin")
    message(FATAL_ERROR
        "Expected default example runtime output dir '${CMAKE_CURRENT_BINARY_DIR}/bin', got '${_lvrs_default_output_dir}'.")
endif()

set(LVRS_EXAMPLE_RUNTIME_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/custom-example-bin")
_lvrs_internal_example_runtime_output_dir(_lvrs_override_output_dir)
if(NOT _lvrs_override_output_dir STREQUAL "${CMAKE_CURRENT_BINARY_DIR}/custom-example-bin")
    message(FATAL_ERROR
        "Expected override example runtime output dir '${CMAKE_CURRENT_BINARY_DIR}/custom-example-bin', got '${_lvrs_override_output_dir}'.")
endif()
