cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_SOURCE_DIR OR LVRS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_SOURCE_DIR is required.")
endif()

set(_lvrs_build_script "${LVRS_SOURCE_DIR}/build.sh")
if(NOT EXISTS "${_lvrs_build_script}")
    message(FATAL_ERROR "Expected root build helper at ${_lvrs_build_script}.")
endif()
if(NOT IS_EXECUTABLE "${_lvrs_build_script}")
    message(FATAL_ERROR "Expected build.sh to be executable: ${_lvrs_build_script}.")
endif()

execute_process(
    COMMAND sh -n "${_lvrs_build_script}"
    RESULT_VARIABLE _lvrs_syntax_result
    OUTPUT_VARIABLE _lvrs_syntax_stdout
    ERROR_VARIABLE _lvrs_syntax_stderr
)
if(NOT _lvrs_syntax_result EQUAL 0)
    message(FATAL_ERROR
        "build.sh failed shell syntax validation.\nstdout:\n${_lvrs_syntax_stdout}\nstderr:\n${_lvrs_syntax_stderr}")
endif()

file(READ "${_lvrs_build_script}" _lvrs_build_script_content)

foreach(_lvrs_required_snippet IN ITEMS
    "BUILD_EXAMPLES=ON"
    "BUILD_TESTS=ON"
    "-DLVRS_BUILD_EXAMPLES="
    "-DLVRS_BUILD_TESTS="
    "lvrs_host_examples_all"
    "Launcher is not a shell script"
    "Missing built example runtime for launcher"
    "Do not override LVRS_BUILD_EXAMPLES/LVRS_BUILD_TESTS"
)
    string(FIND "${_lvrs_build_script_content}" "${_lvrs_required_snippet}" _lvrs_required_index)
    if(_lvrs_required_index EQUAL -1)
        message(FATAL_ERROR
            "Expected build.sh to contain '${_lvrs_required_snippet}'.")
    endif()
endforeach()
