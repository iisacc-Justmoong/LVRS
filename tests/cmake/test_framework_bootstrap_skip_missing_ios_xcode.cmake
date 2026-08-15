cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_SOURCE_DIR OR LVRS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_SOURCE_DIR is required.")
endif()
if(NOT DEFINED LVRS_BINARY_DIR OR LVRS_BINARY_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_BINARY_DIR is required.")
endif()

set(_lvrs_probe_root "${LVRS_BINARY_DIR}/tests/framework-bootstrap-ios-xcode-skip-probe")
set(_lvrs_probe_source_dir "${_lvrs_probe_root}/src")
set(_lvrs_probe_build_dir "${_lvrs_probe_root}/build")
set(_lvrs_fake_bin_dir "${_lvrs_probe_root}/fake-bin")
set(_lvrs_fake_qt_prefix "${_lvrs_probe_root}/fake-qt-ios")

file(REMOVE_RECURSE "${_lvrs_probe_root}")
file(MAKE_DIRECTORY
    "${_lvrs_probe_source_dir}"
    "${_lvrs_fake_bin_dir}"
    "${_lvrs_fake_qt_prefix}/lib/cmake/Qt6"
)
file(WRITE "${_lvrs_fake_qt_prefix}/lib/cmake/Qt6/Qt6Config.cmake" "# fake Qt iOS config\n")

if(WIN32)
    set(_lvrs_path_separator ";")
else()
    set(_lvrs_path_separator ":")
    file(WRITE "${_lvrs_fake_bin_dir}/xcodebuild" "#!/bin/sh\necho \"fake xcodebuild missing full Xcode\" >&2\nexit 1\n")
    file(WRITE "${_lvrs_fake_bin_dir}/xcrun" "#!/bin/sh\necho \"fake xcrun missing iphoneos SDK\" >&2\nexit 1\n")
    file(CHMOD
        "${_lvrs_fake_bin_dir}/xcodebuild"
        "${_lvrs_fake_bin_dir}/xcrun"
        PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
    )
endif()

set(_lvrs_probe_cmakelists [=[
cmake_minimum_required(VERSION 3.21)
project(LVRSFrameworkBootstrapIOSXcodeSkipProbe LANGUAGES NONE)

set(LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS "ios")
set(LVRS_BOOTSTRAP_QT_PREFIX_IOS "@_lvrs_fake_qt_prefix@")

include("@LVRS_SOURCE_DIR@/cmake/LVRSHelpers.cmake")

lvrs_create_framework_bootstrap_targets()
]=])
string(CONFIGURE "${_lvrs_probe_cmakelists}" _lvrs_probe_cmakelists @ONLY)
file(WRITE "${_lvrs_probe_source_dir}/CMakeLists.txt" "${_lvrs_probe_cmakelists}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=${_lvrs_fake_bin_dir}${_lvrs_path_separator}$ENV{PATH}"
        "${CMAKE_COMMAND}" -S "${_lvrs_probe_source_dir}" -B "${_lvrs_probe_build_dir}"
    RESULT_VARIABLE _lvrs_configure_result
    OUTPUT_VARIABLE _lvrs_configure_stdout
    ERROR_VARIABLE _lvrs_configure_stderr
)

if(NOT _lvrs_configure_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to configure framework bootstrap iOS Xcode skip probe.\nstdout:\n${_lvrs_configure_stdout}\nstderr:\n${_lvrs_configure_stderr}")
endif()

set(_lvrs_configure_output "${_lvrs_configure_stdout}\n${_lvrs_configure_stderr}")
string(FIND "${_lvrs_configure_output}" "LVRS framework bootstrap targets: skipping 'ios' (Xcode/iPhoneOS SDK not ready:" _lvrs_skip_message_index)
if(_lvrs_skip_message_index LESS 0)
    message(FATAL_ERROR
        "Expected missing Xcode/iPhoneOS SDK skip status message during framework bootstrap probe configure.\n${_lvrs_configure_output}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${_lvrs_probe_build_dir}" --target help
    RESULT_VARIABLE _lvrs_help_result
    OUTPUT_VARIABLE _lvrs_help_stdout
    ERROR_VARIABLE _lvrs_help_stderr
)

if(NOT _lvrs_help_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to inspect framework bootstrap probe targets.\nstdout:\n${_lvrs_help_stdout}\nstderr:\n${_lvrs_help_stderr}")
endif()

set(_lvrs_help_output "${_lvrs_help_stdout}\n${_lvrs_help_stderr}")

string(FIND "${_lvrs_help_output}" "bootstrap_lvrs_all" _lvrs_bootstrap_all_index)
if(_lvrs_bootstrap_all_index LESS 0)
    message(FATAL_ERROR
        "Expected aggregate framework bootstrap target 'bootstrap_lvrs_all' in help output.\n${_lvrs_help_output}")
endif()

string(FIND "${_lvrs_help_output}" "bootstrap_lvrs_ios" _lvrs_ios_target_index)
if(NOT _lvrs_ios_target_index LESS 0)
    message(FATAL_ERROR
        "Did not expect target 'bootstrap_lvrs_ios' when full Xcode/iPhoneOS SDK is unavailable.\n${_lvrs_help_output}")
endif()
