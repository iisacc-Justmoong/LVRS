cmake_minimum_required(VERSION 3.21)

function(_lvrs_bootstrap_fail message_text)
    message(FATAL_ERROR "LVRS framework bootstrap: ${message_text}")
endfunction()

macro(_lvrs_bootstrap_append_cache_arg cmd_list key value)
    if(NOT "${value}" STREQUAL "")
        string(REPLACE ";" "\\;" _lvrs_cache_value "${value}")
        list(APPEND ${cmd_list} "-D${key}=${_lvrs_cache_value}")
    endif()
endmacro()

function(_lvrs_bootstrap_filter_apple_x86_architectures architectures out_var)
    if(architectures STREQUAL "")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    string(REPLACE "," ";" _lvrs_arch_list "${architectures}")
    set(_lvrs_filtered_arches "")
    foreach(_lvrs_arch IN LISTS _lvrs_arch_list)
        string(STRIP "${_lvrs_arch}" _lvrs_arch)
        if(_lvrs_arch STREQUAL "")
            continue()
        endif()
        string(TOLOWER "${_lvrs_arch}" _lvrs_arch_lower)
        if(_lvrs_arch_lower MATCHES "^(x86|x86_64|amd64|i386)$")
            continue()
        endif()
        list(APPEND _lvrs_filtered_arches "${_lvrs_arch}")
    endforeach()
    list(REMOVE_DUPLICATES _lvrs_filtered_arches)

    if(_lvrs_filtered_arches)
        list(JOIN _lvrs_filtered_arches ";" _lvrs_filtered_joined)
        set(${out_var} "${_lvrs_filtered_joined}" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

function(_lvrs_bootstrap_detect_android_sdk_root out_var)
    if(DEFINED LVRS_BOOTSTRAP_ANDROID_SDK_ROOT
       AND NOT LVRS_BOOTSTRAP_ANDROID_SDK_ROOT STREQUAL ""
       AND IS_DIRECTORY "${LVRS_BOOTSTRAP_ANDROID_SDK_ROOT}")
        set(${out_var} "${LVRS_BOOTSTRAP_ANDROID_SDK_ROOT}" PARENT_SCOPE)
        return()
    endif()

    if(DEFINED ENV{ANDROID_SDK_ROOT} AND IS_DIRECTORY "$ENV{ANDROID_SDK_ROOT}")
        set(${out_var} "$ENV{ANDROID_SDK_ROOT}" PARENT_SCOPE)
        return()
    endif()
    if(DEFINED ENV{ANDROID_HOME} AND IS_DIRECTORY "$ENV{ANDROID_HOME}")
        set(${out_var} "$ENV{ANDROID_HOME}" PARENT_SCOPE)
        return()
    endif()

    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        set(_lvrs_default_sdk "$ENV{HOME}/Library/Android/sdk")
    else()
        set(_lvrs_default_sdk "$ENV{HOME}/Android/Sdk")
    endif()
    if(IS_DIRECTORY "${_lvrs_default_sdk}")
        set(${out_var} "${_lvrs_default_sdk}" PARENT_SCOPE)
        return()
    endif()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

function(_lvrs_bootstrap_detect_android_ndk sdk_root out_var)
    if(DEFINED LVRS_BOOTSTRAP_ANDROID_NDK
       AND NOT LVRS_BOOTSTRAP_ANDROID_NDK STREQUAL ""
       AND IS_DIRECTORY "${LVRS_BOOTSTRAP_ANDROID_NDK}")
        set(${out_var} "${LVRS_BOOTSTRAP_ANDROID_NDK}" PARENT_SCOPE)
        return()
    endif()

    if(DEFINED ENV{CMAKE_ANDROID_NDK} AND IS_DIRECTORY "$ENV{CMAKE_ANDROID_NDK}")
        set(${out_var} "$ENV{CMAKE_ANDROID_NDK}" PARENT_SCOPE)
        return()
    endif()
    if(DEFINED ENV{ANDROID_NDK_ROOT} AND IS_DIRECTORY "$ENV{ANDROID_NDK_ROOT}")
        set(${out_var} "$ENV{ANDROID_NDK_ROOT}" PARENT_SCOPE)
        return()
    endif()
    if(DEFINED ENV{ANDROID_NDK_HOME} AND IS_DIRECTORY "$ENV{ANDROID_NDK_HOME}")
        set(${out_var} "$ENV{ANDROID_NDK_HOME}" PARENT_SCOPE)
        return()
    endif()

    set(_lvrs_common_ndk_candidates
        "/opt/homebrew/share/android-ndk"
        "/usr/local/share/android-ndk"
        "/opt/android/android-ndk-r26b"
    )
    foreach(_lvrs_common_ndk IN LISTS _lvrs_common_ndk_candidates)
        if(IS_DIRECTORY "${_lvrs_common_ndk}")
            set(${out_var} "${_lvrs_common_ndk}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    if(NOT "${sdk_root}" STREQUAL "" AND IS_DIRECTORY "${sdk_root}/ndk")
        file(GLOB _lvrs_ndk_versions LIST_DIRECTORIES true "${sdk_root}/ndk/*")
        set(_lvrs_ndk_dirs)
        foreach(_lvrs_ndk_version_dir IN LISTS _lvrs_ndk_versions)
            if(IS_DIRECTORY "${_lvrs_ndk_version_dir}")
                list(APPEND _lvrs_ndk_dirs "${_lvrs_ndk_version_dir}")
            endif()
        endforeach()
        if(_lvrs_ndk_dirs)
            list(SORT _lvrs_ndk_dirs)
            list(REVERSE _lvrs_ndk_dirs)
            list(GET _lvrs_ndk_dirs 0 _lvrs_selected_ndk)
            set(${out_var} "${_lvrs_selected_ndk}" PARENT_SCOPE)
            return()
        endif()
    endif()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

function(_lvrs_bootstrap_detect_emscripten_toolchain_from_emsdk_root emsdk_root out_var)
    set(_lvrs_toolchain_candidate "")
    if(emsdk_root STREQUAL "" OR NOT IS_DIRECTORY "${emsdk_root}")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    if(EXISTS "${emsdk_root}/.emscripten")
        file(READ "${emsdk_root}/.emscripten" _lvrs_emsdk_config)
        string(REGEX MATCH "EMSCRIPTEN_ROOT.*$" _lvrs_emroot_line "${_lvrs_emsdk_config}")
        string(REGEX MATCH "'([^' ]*)'" _lvrs_emroot_match "${_lvrs_emroot_line}")
        string(REPLACE "'" "" _lvrs_emroot_suffix "${_lvrs_emroot_match}")
        if(NOT _lvrs_emroot_suffix STREQUAL "")
            file(TO_CMAKE_PATH "${_lvrs_emroot_suffix}" _lvrs_emroot_suffix)
            set(_lvrs_toolchain_candidate
                "${emsdk_root}/${_lvrs_emroot_suffix}/cmake/Modules/Platform/Emscripten.cmake")
            if(EXISTS "${_lvrs_toolchain_candidate}")
                set(${out_var} "${_lvrs_toolchain_candidate}" PARENT_SCOPE)
                return()
            endif()
        endif()
    endif()

    set(_lvrs_emscripten_candidates
        "${emsdk_root}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
        "${emsdk_root}/fastcomp/emscripten/cmake/Modules/Platform/Emscripten.cmake"
        "${emsdk_root}/emscripten/cmake/Modules/Platform/Emscripten.cmake"
    )
    foreach(_lvrs_candidate IN LISTS _lvrs_emscripten_candidates)
        if(EXISTS "${_lvrs_candidate}")
            set(${out_var} "${_lvrs_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

function(_lvrs_bootstrap_detect_emsdk_root out_var)
    if(DEFINED LVRS_BOOTSTRAP_EMSDK_ROOT
       AND NOT LVRS_BOOTSTRAP_EMSDK_ROOT STREQUAL ""
       AND IS_DIRECTORY "${LVRS_BOOTSTRAP_EMSDK_ROOT}")
        set(${out_var} "${LVRS_BOOTSTRAP_EMSDK_ROOT}" PARENT_SCOPE)
        return()
    endif()
    if(DEFINED ENV{LVRS_BOOTSTRAP_EMSDK_ROOT}
       AND NOT "$ENV{LVRS_BOOTSTRAP_EMSDK_ROOT}" STREQUAL ""
       AND IS_DIRECTORY "$ENV{LVRS_BOOTSTRAP_EMSDK_ROOT}")
        set(${out_var} "$ENV{LVRS_BOOTSTRAP_EMSDK_ROOT}" PARENT_SCOPE)
        return()
    endif()
    if(DEFINED ENV{EMSDK} AND NOT "$ENV{EMSDK}" STREQUAL "" AND IS_DIRECTORY "$ENV{EMSDK}")
        set(${out_var} "$ENV{EMSDK}" PARENT_SCOPE)
        return()
    endif()

    set(_lvrs_prefix_hints "")
    if(DEFINED LVRS_BOOTSTRAP_PREFIX_PATH AND NOT LVRS_BOOTSTRAP_PREFIX_PATH STREQUAL "")
        foreach(_lvrs_prefix_hint IN LISTS LVRS_BOOTSTRAP_PREFIX_PATH)
            if(NOT _lvrs_prefix_hint STREQUAL "")
                list(APPEND _lvrs_prefix_hints "${_lvrs_prefix_hint}")
            endif()
        endforeach()
    endif()
    if(DEFINED LVRS_BOOTSTRAP_TOOLCHAIN_FILE
       AND NOT LVRS_BOOTSTRAP_TOOLCHAIN_FILE STREQUAL ""
       AND LVRS_BOOTSTRAP_TOOLCHAIN_FILE MATCHES "qt\\.toolchain\\.cmake$")
        get_filename_component(_lvrs_qt6_dir "${LVRS_BOOTSTRAP_TOOLCHAIN_FILE}" DIRECTORY)
        get_filename_component(_lvrs_qt_prefix "${_lvrs_qt6_dir}/../../.." ABSOLUTE)
        if(IS_DIRECTORY "${_lvrs_qt_prefix}")
            list(APPEND _lvrs_prefix_hints "${_lvrs_qt_prefix}")
        endif()
    endif()

    set(_lvrs_emsdk_candidates "")
    foreach(_lvrs_prefix_hint IN LISTS _lvrs_prefix_hints)
        get_filename_component(_lvrs_prefix_abs "${_lvrs_prefix_hint}" ABSOLUTE)
        if(NOT IS_DIRECTORY "${_lvrs_prefix_abs}")
            continue()
        endif()
        get_filename_component(_lvrs_qt_version_root "${_lvrs_prefix_abs}" DIRECTORY)
        get_filename_component(_lvrs_qt_root "${_lvrs_qt_version_root}" DIRECTORY)
        list(APPEND _lvrs_emsdk_candidates
            "${_lvrs_qt_root}/Tools/emsdk"
            "${_lvrs_qt_root}/Tools/Emscripten"
        )
    endforeach()
    list(APPEND _lvrs_emsdk_candidates
        "$ENV{HOME}/emsdk"
        "$ENV{HOME}/.emsdk"
        "/opt/emsdk"
    )
    list(REMOVE_DUPLICATES _lvrs_emsdk_candidates)

    foreach(_lvrs_candidate IN LISTS _lvrs_emsdk_candidates)
        if(NOT IS_DIRECTORY "${_lvrs_candidate}")
            continue()
        endif()
        _lvrs_bootstrap_detect_emscripten_toolchain_from_emsdk_root(
            "${_lvrs_candidate}" _lvrs_candidate_toolchain)
        if(NOT _lvrs_candidate_toolchain STREQUAL "")
            set(${out_var} "${_lvrs_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

if(NOT DEFINED LVRS_BOOTSTRAP_SOURCE_DIR OR LVRS_BOOTSTRAP_SOURCE_DIR STREQUAL "")
    _lvrs_bootstrap_fail("LVRS_BOOTSTRAP_SOURCE_DIR is required.")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_BINARY_DIR OR LVRS_BOOTSTRAP_BINARY_DIR STREQUAL "")
    _lvrs_bootstrap_fail("LVRS_BOOTSTRAP_BINARY_DIR is required.")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_PLATFORM OR LVRS_BOOTSTRAP_PLATFORM STREQUAL "")
    _lvrs_bootstrap_fail("LVRS_BOOTSTRAP_PLATFORM is required.")
endif()

if(NOT DEFINED LVRS_BOOTSTRAP_SYSTEM_NAME OR LVRS_BOOTSTRAP_SYSTEM_NAME STREQUAL "")
    set(LVRS_BOOTSTRAP_SYSTEM_NAME "Unknown")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_BUILD_TYPE OR LVRS_BOOTSTRAP_BUILD_TYPE STREQUAL "")
    set(LVRS_BOOTSTRAP_BUILD_TYPE "Release")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_INSTALL_PREFIX OR LVRS_BOOTSTRAP_INSTALL_PREFIX STREQUAL "")
    set(LVRS_BOOTSTRAP_INSTALL_PREFIX "${LVRS_BOOTSTRAP_BINARY_DIR}/install")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES)
    set(LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES "OFF")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_LVRS_BUILD_TESTS)
    set(LVRS_BOOTSTRAP_LVRS_BUILD_TESTS "OFF")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS)
    set(LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS "ON")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_LVRS_INSTALL_QML_MODULE)
    set(LVRS_BOOTSTRAP_LVRS_INSTALL_QML_MODULE "ON")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_LVRS_ENFORCE_VULKAN)
    set(LVRS_BOOTSTRAP_LVRS_ENFORCE_VULKAN "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_FIND_PACKAGE_NO_PACKAGE_REGISTRY)
    set(LVRS_BOOTSTRAP_FIND_PACKAGE_NO_PACKAGE_REGISTRY "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_FIND_USE_PACKAGE_REGISTRY)
    set(LVRS_BOOTSTRAP_FIND_USE_PACKAGE_REGISTRY "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_ANDROID_SDK_ROOT)
    set(LVRS_BOOTSTRAP_ANDROID_SDK_ROOT "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_ANDROID_NDK)
    set(LVRS_BOOTSTRAP_ANDROID_NDK "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_IOS_ARCHITECTURES)
    set(LVRS_BOOTSTRAP_IOS_ARCHITECTURES "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_IOS_CODE_SIGNING)
    set(LVRS_BOOTSTRAP_IOS_CODE_SIGNING "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_IOS_BUNDLE_IDENTIFIER)
    set(LVRS_BOOTSTRAP_IOS_BUNDLE_IDENTIFIER "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_EMSDK_ROOT)
    set(LVRS_BOOTSTRAP_EMSDK_ROOT "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE)
    set(LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE "")
endif()

if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "ios")
    string(TOUPPER "${LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS}" _lvrs_ios_shared_libs_upper)
    if(NOT _lvrs_ios_shared_libs_upper MATCHES "^(0|OFF|NO|FALSE)$")
        message(STATUS "LVRS framework bootstrap: forcing LVRS_BUILD_SHARED_LIBS=OFF for iOS.")
        set(LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS "OFF")
    endif()

    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        _lvrs_bootstrap_filter_apple_x86_architectures(
            "${LVRS_BOOTSTRAP_IOS_ARCHITECTURES}"
            _lvrs_ios_architectures_filtered
        )
        if(NOT LVRS_BOOTSTRAP_IOS_ARCHITECTURES STREQUAL "${_lvrs_ios_architectures_filtered}")
            message(STATUS "LVRS framework bootstrap: removed Apple x86 iOS architectures.")
        endif()
        if(_lvrs_ios_architectures_filtered STREQUAL "")
            set(LVRS_BOOTSTRAP_IOS_ARCHITECTURES "arm64")
        else()
            set(LVRS_BOOTSTRAP_IOS_ARCHITECTURES "${_lvrs_ios_architectures_filtered}")
        endif()
    endif()

    if(LVRS_BOOTSTRAP_IOS_CODE_SIGNING STREQUAL "")
        set(LVRS_BOOTSTRAP_IOS_CODE_SIGNING "OFF")
    endif()

    if(LVRS_BOOTSTRAP_IOS_BUNDLE_IDENTIFIER STREQUAL "")
        set(LVRS_BOOTSTRAP_IOS_BUNDLE_IDENTIFIER "com.lvrs.framework")
    endif()
endif()

if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "wasm")
    string(TOUPPER "${LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS}" _lvrs_wasm_shared_libs_upper)
    if(NOT _lvrs_wasm_shared_libs_upper MATCHES "^(0|OFF|NO|FALSE)$")
        message(STATUS "LVRS framework bootstrap: forcing LVRS_BUILD_SHARED_LIBS=OFF for WASM.")
        set(LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS "OFF")
    endif()

    if(LVRS_BOOTSTRAP_EMSDK_ROOT STREQUAL "")
        _lvrs_bootstrap_detect_emsdk_root(_lvrs_detected_emsdk_root)
        set(LVRS_BOOTSTRAP_EMSDK_ROOT "${_lvrs_detected_emsdk_root}")
    endif()
    if(NOT LVRS_BOOTSTRAP_EMSDK_ROOT STREQUAL "")
        set(ENV{EMSDK} "${LVRS_BOOTSTRAP_EMSDK_ROOT}")
    endif()

    if(LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE STREQUAL "")
        _lvrs_bootstrap_detect_emscripten_toolchain_from_emsdk_root(
            "${LVRS_BOOTSTRAP_EMSDK_ROOT}" _lvrs_detected_emscripten_toolchain)
        set(LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE "${_lvrs_detected_emscripten_toolchain}")
    endif()
    if(NOT LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE STREQUAL ""
       AND NOT EXISTS "${LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE}")
        _lvrs_bootstrap_fail("configured LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE does not exist: ${LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE}")
    endif()

    set(_lvrs_wasm_uses_qt_toolchain FALSE)
    if(NOT LVRS_BOOTSTRAP_TOOLCHAIN_FILE STREQUAL ""
       AND LVRS_BOOTSTRAP_TOOLCHAIN_FILE MATCHES "qt\\.toolchain\\.cmake$")
        set(_lvrs_wasm_uses_qt_toolchain TRUE)
    endif()

    if(_lvrs_wasm_uses_qt_toolchain AND LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE STREQUAL "")
        _lvrs_bootstrap_fail("cannot resolve Emscripten.cmake for Qt WASM toolchain. Set LVRS_BOOTSTRAP_EMSDK_ROOT or LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE or export EMSDK to a valid emsdk root.")
    endif()
endif()

file(MAKE_DIRECTORY "${LVRS_BOOTSTRAP_BINARY_DIR}")
file(MAKE_DIRECTORY "${LVRS_BOOTSTRAP_INSTALL_PREFIX}")

if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "android")
    if(NOT LVRS_BOOTSTRAP_ANDROID_SDK_ROOT STREQUAL ""
       AND NOT IS_DIRECTORY "${LVRS_BOOTSTRAP_ANDROID_SDK_ROOT}")
        _lvrs_bootstrap_fail("configured LVRS_BOOTSTRAP_ANDROID_SDK_ROOT does not exist: ${LVRS_BOOTSTRAP_ANDROID_SDK_ROOT}")
    endif()
    if(NOT LVRS_BOOTSTRAP_ANDROID_NDK STREQUAL ""
       AND NOT IS_DIRECTORY "${LVRS_BOOTSTRAP_ANDROID_NDK}")
        _lvrs_bootstrap_fail("configured LVRS_BOOTSTRAP_ANDROID_NDK does not exist: ${LVRS_BOOTSTRAP_ANDROID_NDK}")
    endif()
    if(LVRS_BOOTSTRAP_ANDROID_SDK_ROOT STREQUAL "")
        _lvrs_bootstrap_detect_android_sdk_root(_lvrs_detected_android_sdk)
        set(LVRS_BOOTSTRAP_ANDROID_SDK_ROOT "${_lvrs_detected_android_sdk}")
    endif()
    if(LVRS_BOOTSTRAP_ANDROID_NDK STREQUAL "")
        _lvrs_bootstrap_detect_android_ndk("${LVRS_BOOTSTRAP_ANDROID_SDK_ROOT}" _lvrs_detected_android_ndk)
        set(LVRS_BOOTSTRAP_ANDROID_NDK "${_lvrs_detected_android_ndk}")
    endif()
endif()

set(_lvrs_configure_cmd "${CMAKE_COMMAND}")
if(DEFINED LVRS_BOOTSTRAP_GENERATOR AND NOT LVRS_BOOTSTRAP_GENERATOR STREQUAL "")
    list(APPEND _lvrs_configure_cmd -G "${LVRS_BOOTSTRAP_GENERATOR}")
endif()
list(APPEND _lvrs_configure_cmd
    -S "${LVRS_BOOTSTRAP_SOURCE_DIR}"
    -B "${LVRS_BOOTSTRAP_BINARY_DIR}"
)
if(NOT LVRS_BOOTSTRAP_SYSTEM_NAME STREQUAL "Unknown")
    _lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_SYSTEM_NAME" "${LVRS_BOOTSTRAP_SYSTEM_NAME}")
endif()
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_PREFIX_PATH" "${LVRS_BOOTSTRAP_PREFIX_PATH}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_TOOLCHAIN_FILE" "${LVRS_BOOTSTRAP_TOOLCHAIN_FILE}")
if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "wasm")
    _lvrs_bootstrap_append_cache_arg(
        _lvrs_configure_cmd
        "QT_CHAINLOAD_TOOLCHAIN_FILE"
        "${LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE}")
endif()
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_BUILD_TYPE" "${LVRS_BOOTSTRAP_BUILD_TYPE}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_OSX_SYSROOT" "${LVRS_BOOTSTRAP_OSX_SYSROOT}")
if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "ios")
    _lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_OSX_ARCHITECTURES" "${LVRS_BOOTSTRAP_IOS_ARCHITECTURES}")
    _lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER" "${LVRS_BOOTSTRAP_IOS_BUNDLE_IDENTIFIER}")
    string(TOUPPER "${LVRS_BOOTSTRAP_IOS_CODE_SIGNING}" _lvrs_ios_code_signing_upper)
    if(_lvrs_ios_code_signing_upper MATCHES "^(0|OFF|NO|FALSE)$")
        _lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED" "NO")
        _lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED" "NO")
    endif()
endif()
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_ANDROID_ARCH_ABI" "${LVRS_BOOTSTRAP_ANDROID_ABI}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_ANDROID_NDK" "${LVRS_BOOTSTRAP_ANDROID_NDK}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "ANDROID_NDK_ROOT" "${LVRS_BOOTSTRAP_ANDROID_NDK}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_ANDROID_SDK" "${LVRS_BOOTSTRAP_ANDROID_SDK_ROOT}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "ANDROID_SDK_ROOT" "${LVRS_BOOTSTRAP_ANDROID_SDK_ROOT}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "ANDROID_HOME" "${LVRS_BOOTSTRAP_ANDROID_SDK_ROOT}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY" "${LVRS_BOOTSTRAP_FIND_PACKAGE_NO_PACKAGE_REGISTRY}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_FIND_USE_PACKAGE_REGISTRY" "${LVRS_BOOTSTRAP_FIND_USE_PACKAGE_REGISTRY}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "LVRS_BUILD_EXAMPLES" "${LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "LVRS_BUILD_TESTS" "${LVRS_BOOTSTRAP_LVRS_BUILD_TESTS}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "LVRS_BUILD_SHARED_LIBS" "${LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "LVRS_INSTALL_QML_MODULE" "${LVRS_BOOTSTRAP_LVRS_INSTALL_QML_MODULE}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "LVRS_ENFORCE_VULKAN" "${LVRS_BOOTSTRAP_LVRS_ENFORCE_VULKAN}")
list(APPEND _lvrs_configure_cmd "-DLVRS_ENABLE_FRAMEWORK_BOOTSTRAP_TARGETS=OFF")

message(STATUS "LVRS framework bootstrap: configure '${LVRS_BOOTSTRAP_PLATFORM}' -> ${LVRS_BOOTSTRAP_BINARY_DIR}")
execute_process(
    COMMAND ${_lvrs_configure_cmd}
    RESULT_VARIABLE _lvrs_configure_result
    COMMAND_ECHO STDOUT
)
if(NOT _lvrs_configure_result EQUAL 0)
    _lvrs_bootstrap_fail("configure failed for platform '${LVRS_BOOTSTRAP_PLATFORM}' (exit=${_lvrs_configure_result}).")
endif()

set(_lvrs_build_cmd
    "${CMAKE_COMMAND}"
    --build "${LVRS_BOOTSTRAP_BINARY_DIR}"
    --parallel 1
)
if(NOT LVRS_BOOTSTRAP_BUILD_TYPE STREQUAL "")
    list(APPEND _lvrs_build_cmd --config "${LVRS_BOOTSTRAP_BUILD_TYPE}")
endif()
set(_lvrs_build_env_cmd
    "${CMAKE_COMMAND}"
    -E env
    "MAKEFLAGS="
    "MFLAGS="
    "CMAKE_BUILD_PARALLEL_LEVEL="
)

message(STATUS "LVRS framework bootstrap: build default targets for '${LVRS_BOOTSTRAP_PLATFORM}'")
execute_process(
    COMMAND ${_lvrs_build_env_cmd} ${_lvrs_build_cmd}
    RESULT_VARIABLE _lvrs_build_result
    COMMAND_ECHO STDOUT
)
if(NOT _lvrs_build_result EQUAL 0)
    _lvrs_bootstrap_fail("build failed for platform '${LVRS_BOOTSTRAP_PLATFORM}' (exit=${_lvrs_build_result}).")
endif()

set(_lvrs_install_cmd
    "${CMAKE_COMMAND}"
    --install "${LVRS_BOOTSTRAP_BINARY_DIR}"
    --prefix "${LVRS_BOOTSTRAP_INSTALL_PREFIX}"
)
if(NOT LVRS_BOOTSTRAP_BUILD_TYPE STREQUAL "")
    list(APPEND _lvrs_install_cmd --config "${LVRS_BOOTSTRAP_BUILD_TYPE}")
endif()

message(STATUS "LVRS framework bootstrap: install '${LVRS_BOOTSTRAP_PLATFORM}' -> ${LVRS_BOOTSTRAP_INSTALL_PREFIX}")
execute_process(
    COMMAND ${_lvrs_install_cmd}
    RESULT_VARIABLE _lvrs_install_result
    COMMAND_ECHO STDOUT
)
if(NOT _lvrs_install_result EQUAL 0)
    _lvrs_bootstrap_fail("install failed for platform '${LVRS_BOOTSTRAP_PLATFORM}' (exit=${_lvrs_install_result}).")
endif()

if(NOT EXISTS "${LVRS_BOOTSTRAP_INSTALL_PREFIX}/lib/cmake/LVRS/LVRSConfig.cmake"
   AND NOT EXISTS "${LVRS_BOOTSTRAP_INSTALL_PREFIX}/LVRSConfig.cmake")
    _lvrs_bootstrap_fail("installed package config not found under '${LVRS_BOOTSTRAP_INSTALL_PREFIX}'.")
endif()

if(LVRS_BOOTSTRAP_INSTALL_PREFIX MATCHES "/platforms/${LVRS_BOOTSTRAP_PLATFORM}$")
    get_filename_component(_lvrs_platforms_root "${LVRS_BOOTSTRAP_INSTALL_PREFIX}" DIRECTORY)
    get_filename_component(_lvrs_install_root "${_lvrs_platforms_root}" DIRECTORY)
    set(_lvrs_platform_config "${LVRS_BOOTSTRAP_INSTALL_PREFIX}/lib/cmake/LVRS/LVRSConfig.cmake")
    set(_lvrs_platform_config_version "${LVRS_BOOTSTRAP_INSTALL_PREFIX}/lib/cmake/LVRS/LVRSConfigVersion.cmake")
    if(EXISTS "${_lvrs_platform_config}")
        file(COPY_FILE "${_lvrs_platform_config}" "${_lvrs_install_root}/LVRSConfig.cmake" ONLY_IF_DIFFERENT)
    endif()
    if(EXISTS "${_lvrs_platform_config_version}")
        file(COPY_FILE "${_lvrs_platform_config_version}" "${_lvrs_install_root}/LVRSConfigVersion.cmake" ONLY_IF_DIFFERENT)
    endif()
endif()

message(STATUS "LVRS framework bootstrap: install completed for '${LVRS_BOOTSTRAP_PLATFORM}'")
