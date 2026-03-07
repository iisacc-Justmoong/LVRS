cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_BINARY_DIR OR LVRS_BINARY_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_BINARY_DIR is required.")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${LVRS_BINARY_DIR}" --target help
    RESULT_VARIABLE _lvrs_help_result
    OUTPUT_VARIABLE _lvrs_help_stdout
    ERROR_VARIABLE _lvrs_help_stderr
)

if(NOT _lvrs_help_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to inspect build targets.\nstdout:\n${_lvrs_help_stdout}\nstderr:\n${_lvrs_help_stderr}")
endif()

set(_lvrs_help_output "${_lvrs_help_stdout}\n${_lvrs_help_stderr}")

string(FIND "${_lvrs_help_output}" "lvrs_host_examples_all" _lvrs_host_examples_index)
if(_lvrs_host_examples_index LESS 0)
    message(FATAL_ERROR
        "Expected aggregate host example target 'lvrs_host_examples_all' in build help output.\n${_lvrs_help_output}")
endif()

string(FIND "${_lvrs_help_output}" "LVRSExampleVisualCatalog" _lvrs_visual_catalog_index)
if(_lvrs_visual_catalog_index LESS 0)
    message(FATAL_ERROR
        "Expected example target 'LVRSExampleVisualCatalog' in build help output.\n${_lvrs_help_output}")
endif()
