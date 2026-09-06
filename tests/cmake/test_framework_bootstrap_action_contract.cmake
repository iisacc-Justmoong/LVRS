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

# Exercise parent target generation, the bootstrap action, and its child configure.
# A relocated target kit cannot rely on Qt's original absolute host-tools path.
set(_lvrs_probe_root "${LVRS_SOURCE_DIR}/build/tests/framework-qt-host-path")
file(REMOVE_RECURSE "${_lvrs_probe_root}")
file(MAKE_DIRECTORY "${_lvrs_probe_root}/parent" "${_lvrs_probe_root}/child")
set(_lvrs_detected_host "${_lvrs_probe_root}/detected host Qt")
set(_lvrs_explicit_host "${_lvrs_probe_root}/explicit host Qt")
foreach(_lvrs_host IN ITEMS "${_lvrs_detected_host}" "${_lvrs_explicit_host}")
    file(MAKE_DIRECTORY "${_lvrs_host}/lib/cmake/Qt6")
    file(WRITE "${_lvrs_host}/lib/cmake/Qt6/Qt6Config.cmake" "set(Qt6_FOUND TRUE)\n")
endforeach()
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(_lvrs_probe_platform macos)
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(_lvrs_probe_platform windows)
else()
    set(_lvrs_probe_platform linux)
endif()
string(TOUPPER "${_lvrs_probe_platform}" _lvrs_probe_platform_upper)

set(_lvrs_parent_source [=[
cmake_minimum_required(VERSION 3.21)
project(LVRSHostQtParent LANGUAGES NONE)
set(Qt6_DIR "@_lvrs_detected_host@/lib/cmake/Qt6")
set(LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS "@_lvrs_probe_platform@")
set(LVRS_BOOTSTRAP_QT_PREFIX_@_lvrs_probe_platform_upper@ "@_lvrs_detected_host@")
set(LVRS_BOOTSTRAP_SOURCE_DIR "@_lvrs_probe_root@/child")
set(LVRS_BOOTSTRAP_INSTALL_ROOT "${CMAKE_BINARY_DIR}/installed")
include("@LVRS_SOURCE_DIR@/cmake/LVRSHelpers.cmake")
lvrs_create_framework_bootstrap_targets()
]=])
string(CONFIGURE "${_lvrs_parent_source}" _lvrs_parent_source @ONLY)
file(WRITE "${_lvrs_probe_root}/parent/CMakeLists.txt" "${_lvrs_parent_source}")
file(WRITE "${_lvrs_probe_root}/child/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.21)
project(LVRSHostQtChild LANGUAGES NONE)
if(NOT DEFINED QT_HOST_PATH OR QT_HOST_PATH STREQUAL "")
    message(FATAL_ERROR "The parent host Qt path was lost")
endif()
find_package(Qt6 CONFIG REQUIRED PATHS "${QT_HOST_PATH}/lib/cmake/Qt6" NO_DEFAULT_PATH)
file(WRITE "${CMAKE_BINARY_DIR}/host-qt.txt" "${QT_HOST_PATH}")
add_custom_target(LVRSCore)
install(FILES "${CMAKE_BINARY_DIR}/host-qt.txt" DESTINATION .)
file(WRITE "${CMAKE_BINARY_DIR}/LVRSConfig.cmake" "set(LVRS_FOUND TRUE)\n")
install(FILES "${CMAKE_BINARY_DIR}/LVRSConfig.cmake" DESTINATION lib/cmake/LVRS)
]=])

foreach(_lvrs_case IN ITEMS detected explicit standard)
    set(_lvrs_expected_host "${_lvrs_detected_host}")
    set(_lvrs_override_args "")
    if(_lvrs_case STREQUAL "explicit")
        set(_lvrs_expected_host "${_lvrs_explicit_host}")
        list(APPEND _lvrs_override_args "-DLVRS_BOOTSTRAP_QT_HOST_PREFIX=${_lvrs_explicit_host}")
    elseif(_lvrs_case STREQUAL "standard")
        set(_lvrs_expected_host "${_lvrs_explicit_host}")
        list(APPEND _lvrs_override_args "-DQT_HOST_PATH=${_lvrs_explicit_host}")
    endif()
    set(_lvrs_probe_build "${_lvrs_probe_root}/${_lvrs_case}/build")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            --unset=LVRS_BOOTSTRAP_QT_HOST_PREFIX --unset=QT_HOST_PATH
            "${CMAKE_COMMAND}" -S "${_lvrs_probe_root}/parent" -B "${_lvrs_probe_build}"
            ${_lvrs_override_args}
        RESULT_VARIABLE _lvrs_result OUTPUT_VARIABLE _lvrs_stdout ERROR_VARIABLE _lvrs_stderr)
    if(NOT _lvrs_result EQUAL 0)
        message(FATAL_ERROR "Host Qt parent configure failed: ${_lvrs_stdout}\n${_lvrs_stderr}")
    endif()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${_lvrs_probe_build}" --target bootstrap_lvrs_all
        RESULT_VARIABLE _lvrs_result OUTPUT_VARIABLE _lvrs_stdout ERROR_VARIABLE _lvrs_stderr)
    if(NOT _lvrs_result EQUAL 0)
        message(FATAL_ERROR "Host Qt bootstrap failed: ${_lvrs_stdout}\n${_lvrs_stderr}")
    endif()
    file(READ "${_lvrs_probe_build}/installed/${_lvrs_probe_platform}/host-qt.txt" _lvrs_actual_host)
    if(NOT _lvrs_actual_host STREQUAL _lvrs_expected_host)
        message(FATAL_ERROR "Host Qt mismatch: expected ${_lvrs_expected_host}, got ${_lvrs_actual_host}")
    endif()
endforeach()
