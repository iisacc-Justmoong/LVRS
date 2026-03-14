cmake_minimum_required(VERSION 3.21)

if(NOT DEFINED LVRS_SOURCE_DIR OR LVRS_SOURCE_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_SOURCE_DIR is required.")
endif()
if(NOT DEFINED LVRS_BINARY_DIR OR LVRS_BINARY_DIR STREQUAL "")
    message(FATAL_ERROR "LVRS_BINARY_DIR is required.")
endif()

set(_lvrs_probe_root "${LVRS_BINARY_DIR}/tests/linux-runtime-bundle-probe")
set(_lvrs_probe_source_dir "${_lvrs_probe_root}/src")
set(_lvrs_probe_build_dir "${_lvrs_probe_root}/build")
set(_lvrs_probe_runtime_root "${_lvrs_probe_root}/runtime")
set(_lvrs_probe_lib "${_lvrs_probe_runtime_root}/libLVRS.so")
set(_lvrs_probe_qml_module_dir "${_lvrs_probe_runtime_root}/qml/LVRS")
set(_lvrs_probe_main_cpp "${_lvrs_probe_source_dir}/main.cpp")
set(_lvrs_probe_result_file "${_lvrs_probe_build_dir}/linux_runtime_bundle_result.txt")

file(REMOVE_RECURSE "${_lvrs_probe_root}")
file(MAKE_DIRECTORY "${_lvrs_probe_source_dir}")
file(MAKE_DIRECTORY "${_lvrs_probe_qml_module_dir}")
file(WRITE "${_lvrs_probe_lib}" "fake-shared-library")
file(WRITE "${_lvrs_probe_qml_module_dir}/qmldir" "module LVRS\n")
file(WRITE "${_lvrs_probe_main_cpp}" "int main() { return 0; }\n")

set(LVRS_PROBE_LIB "${_lvrs_probe_lib}")
set(LVRS_PROBE_QML_MODULE_DIR "${_lvrs_probe_qml_module_dir}")
set(LVRS_PROBE_MAIN_CPP "${_lvrs_probe_main_cpp}")
set(LVRS_PROBE_RESULT_FILE "${_lvrs_probe_result_file}")

set(_lvrs_probe_cmakelists [=[
cmake_minimum_required(VERSION 3.21)
project(LVRSLinuxRuntimeBundleProbe LANGUAGES CXX)

include("@LVRS_SOURCE_DIR@/cmake/LVRSHelpers.cmake")

add_library(LVRS::LVRS SHARED IMPORTED GLOBAL)
set_target_properties(LVRS::LVRS PROPERTIES
    IMPORTED_LOCATION "@LVRS_PROBE_LIB@"
)
set(LVRS_QML_MODULE_PATH "@LVRS_PROBE_QML_MODULE_DIR@")

add_executable(ProbeApp "@LVRS_PROBE_MAIN_CPP@")
_lvrs_internal_apply_linux_runtime_bundle(ProbeApp)

get_target_property(_lvrs_applied ProbeApp _LVRS_LINUX_RUNTIME_BUNDLE_APPLIED)
get_target_property(_lvrs_bundle_dir ProbeApp _LVRS_LINUX_RUNTIME_BUNDLE_DIR)
get_target_property(_lvrs_bundle_rpath ProbeApp _LVRS_LINUX_RUNTIME_BUNDLE_RPATH)
get_target_property(_lvrs_bundle_lib_target ProbeApp _LVRS_LINUX_RUNTIME_BUNDLE_LIBRARY_TARGET)
get_target_property(_lvrs_bundle_qml_source ProbeApp _LVRS_LINUX_RUNTIME_BUNDLE_QML_SOURCE)
get_target_property(_lvrs_build_rpath ProbeApp BUILD_RPATH)
get_target_property(_lvrs_install_rpath ProbeApp INSTALL_RPATH)

file(WRITE "@LVRS_PROBE_RESULT_FILE@"
    "applied=${_lvrs_applied}\n"
    "bundle_dir=${_lvrs_bundle_dir}\n"
    "bundle_rpath=${_lvrs_bundle_rpath}\n"
    "bundle_lib_target=${_lvrs_bundle_lib_target}\n"
    "bundle_qml_source=${_lvrs_bundle_qml_source}\n"
    "build_rpath=${_lvrs_build_rpath}\n"
    "install_rpath=${_lvrs_install_rpath}\n"
)
]=])
string(CONFIGURE "${_lvrs_probe_cmakelists}" _lvrs_probe_cmakelists @ONLY)
file(WRITE "${_lvrs_probe_source_dir}/CMakeLists.txt" "${_lvrs_probe_cmakelists}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${_lvrs_probe_source_dir}" -B "${_lvrs_probe_build_dir}"
    RESULT_VARIABLE _lvrs_configure_result
    OUTPUT_VARIABLE _lvrs_configure_stdout
    ERROR_VARIABLE _lvrs_configure_stderr
)

if(NOT _lvrs_configure_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to configure Linux runtime bundle probe.\nstdout:\n${_lvrs_configure_stdout}\nstderr:\n${_lvrs_configure_stderr}")
endif()

if(NOT EXISTS "${_lvrs_probe_result_file}")
    message(FATAL_ERROR "Linux runtime bundle probe did not write result file: ${_lvrs_probe_result_file}")
endif()

file(READ "${_lvrs_probe_result_file}" _lvrs_probe_result)

foreach(_lvrs_expected_line
    IN ITEMS
        "applied=TRUE"
        "bundle_dir=$<TARGET_FILE_DIR:ProbeApp>/lvrs-runtime"
        "bundle_rpath=$ORIGIN/lvrs-runtime"
        "bundle_lib_target=LVRS::LVRS"
        "bundle_qml_source=${_lvrs_probe_qml_module_dir}"
        "build_rpath=$ORIGIN/lvrs-runtime"
        "install_rpath=$ORIGIN/lvrs-runtime"
)
    string(FIND "${_lvrs_probe_result}" "${_lvrs_expected_line}" _lvrs_expected_line_index)
    if(_lvrs_expected_line_index LESS 0)
        message(FATAL_ERROR
            "Missing expected Linux runtime bundle line '${_lvrs_expected_line}'.\n${_lvrs_probe_result}")
    endif()
endforeach()

file(GLOB_RECURSE _lvrs_probe_build_files "${_lvrs_probe_build_dir}/*")
set(_lvrs_found_copy_library FALSE)
set(_lvrs_found_copy_qml FALSE)
foreach(_lvrs_probe_file IN LISTS _lvrs_probe_build_files)
    if(IS_DIRECTORY "${_lvrs_probe_file}")
        continue()
    endif()
    if(NOT _lvrs_probe_file MATCHES "(build\\.ninja|Makefile|\\.cmake$|\\.txt$|\\.make$|\\.ninja$)")
        continue()
    endif()
    file(READ "${_lvrs_probe_file}" _lvrs_probe_file_content)

    if(NOT _lvrs_found_copy_library)
        string(FIND "${_lvrs_probe_file_content}" "${_lvrs_probe_lib}" _lvrs_copy_library_index)
        if(NOT _lvrs_copy_library_index LESS 0)
            set(_lvrs_found_copy_library TRUE)
        endif()
    endif()

    if(NOT _lvrs_found_copy_qml)
        string(FIND "${_lvrs_probe_file_content}" "${_lvrs_probe_qml_module_dir}" _lvrs_copy_qml_index)
        if(NOT _lvrs_copy_qml_index LESS 0)
            set(_lvrs_found_copy_qml TRUE)
        endif()
    endif()
endforeach()

if(NOT _lvrs_found_copy_library)
    message(FATAL_ERROR
        "Linux runtime bundle probe did not emit any build rule referencing ${_lvrs_probe_lib}.")
endif()
if(NOT _lvrs_found_copy_qml)
    message(FATAL_ERROR
        "Linux runtime bundle probe did not emit any build rule referencing ${_lvrs_probe_qml_module_dir}.")
endif()
