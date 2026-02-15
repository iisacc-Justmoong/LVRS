cmake_minimum_required(VERSION 3.21)

function(_lvrs_bootstrap_fail message_text)
    message(FATAL_ERROR "LVRS bootstrap: ${message_text}")
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

function(_lvrs_bootstrap_append_desktop_matches root_dir app_target platform out_var)
    if(NOT IS_DIRECTORY "${root_dir}")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(_lvrs_matches)
    file(GLOB_RECURSE _lvrs_candidates "${root_dir}/*")
    foreach(_lvrs_candidate IN LISTS _lvrs_candidates)
        if(IS_DIRECTORY "${_lvrs_candidate}")
            continue()
        endif()
        if(_lvrs_candidate MATCHES "/CMakeFiles/")
            continue()
        endif()

        get_filename_component(_lvrs_name "${_lvrs_candidate}" NAME)
        if(platform STREQUAL "windows")
            if(NOT _lvrs_name STREQUAL "${app_target}.exe")
                continue()
            endif()
            list(APPEND _lvrs_matches "${_lvrs_candidate}")
        else()
            if(NOT _lvrs_name STREQUAL "${app_target}")
                continue()
            endif()
            if(platform STREQUAL "macos")
                if(_lvrs_candidate MATCHES "\\.app/Contents/MacOS/"
                   OR _lvrs_candidate MATCHES "/bin/"
                   OR _lvrs_candidate MATCHES "/${app_target}$")
                    list(APPEND _lvrs_matches "${_lvrs_candidate}")
                endif()
            else()
                if(_lvrs_candidate MATCHES "/bin/" OR _lvrs_candidate MATCHES "/${app_target}$")
                    list(APPEND _lvrs_matches "${_lvrs_candidate}")
                endif()
            endif()
        endif()
    endforeach()

    set(${out_var} "${_lvrs_matches}" PARENT_SCOPE)
endfunction()

function(_lvrs_bootstrap_find_desktop_executable build_dir host_build_dir app_target platform out_var)
    set(_lvrs_roots)
    list(APPEND _lvrs_roots "${build_dir}")
    if(NOT "${host_build_dir}" STREQUAL "")
        list(APPEND _lvrs_roots "${host_build_dir}")
    endif()
    list(REMOVE_DUPLICATES _lvrs_roots)

    set(_lvrs_matches)
    foreach(_lvrs_root IN LISTS _lvrs_roots)
        _lvrs_bootstrap_append_desktop_matches("${_lvrs_root}" "${app_target}" "${platform}" _lvrs_root_matches)
        if(_lvrs_root_matches)
            list(APPEND _lvrs_matches ${_lvrs_root_matches})
        endif()
    endforeach()

    if(NOT _lvrs_matches)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    list(REMOVE_DUPLICATES _lvrs_matches)
    list(SORT _lvrs_matches)
    list(GET _lvrs_matches 0 _lvrs_selected)
    set(${out_var} "${_lvrs_selected}" PARENT_SCOPE)
endfunction()

function(_lvrs_bootstrap_find_wasm_entry_html build_dir app_target out_var)
    set(_lvrs_candidates)
    file(GLOB_RECURSE _lvrs_html_candidates "${build_dir}/*.html")

    foreach(_lvrs_candidate IN LISTS _lvrs_html_candidates)
        if(NOT IS_DIRECTORY "${_lvrs_candidate}")
            if(_lvrs_candidate MATCHES "/CMakeFiles/")
                continue()
            endif()
            list(APPEND _lvrs_candidates "${_lvrs_candidate}")
        endif()
    endforeach()

    if(NOT _lvrs_candidates)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    foreach(_lvrs_candidate IN LISTS _lvrs_candidates)
        get_filename_component(_lvrs_name "${_lvrs_candidate}" NAME)
        if(_lvrs_name STREQUAL "${app_target}.html")
            set(${out_var} "${_lvrs_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    list(SORT _lvrs_candidates)
    list(GET _lvrs_candidates 0 _lvrs_selected)
    set(${out_var} "${_lvrs_selected}" PARENT_SCOPE)
endfunction()

function(_lvrs_bootstrap_find_ios_app_bundle build_dir app_target out_var)
    set(_lvrs_matches)
    file(GLOB_RECURSE _lvrs_candidates LIST_DIRECTORIES true "${build_dir}/*.app")

    foreach(_lvrs_candidate IN LISTS _lvrs_candidates)
        if(NOT IS_DIRECTORY "${_lvrs_candidate}")
            continue()
        endif()
        get_filename_component(_lvrs_name "${_lvrs_candidate}" NAME)
        if(NOT _lvrs_name STREQUAL "${app_target}.app")
            continue()
        endif()
        list(APPEND _lvrs_matches "${_lvrs_candidate}")
    endforeach()

    if(NOT _lvrs_matches)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    foreach(_lvrs_candidate IN LISTS _lvrs_matches)
        if(_lvrs_candidate MATCHES "iphonesimulator")
            set(${out_var} "${_lvrs_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    list(SORT _lvrs_matches)
    list(GET _lvrs_matches 0 _lvrs_selected)
    set(${out_var} "${_lvrs_selected}" PARENT_SCOPE)
endfunction()

function(_lvrs_bootstrap_find_xcode_project build_dir app_target out_var)
    file(GLOB _lvrs_candidates "${build_dir}/*.xcodeproj")
    set(_lvrs_first_candidate "")

    foreach(_lvrs_candidate IN LISTS _lvrs_candidates)
        if(NOT IS_DIRECTORY "${_lvrs_candidate}")
            continue()
        endif()
        if(_lvrs_first_candidate STREQUAL "")
            set(_lvrs_first_candidate "${_lvrs_candidate}")
        endif()
        get_filename_component(_lvrs_name "${_lvrs_candidate}" NAME)
        if(_lvrs_name STREQUAL "${app_target}.xcodeproj")
            set(${out_var} "${_lvrs_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    list(LENGTH _lvrs_candidates _lvrs_candidate_count)
    if(_lvrs_candidate_count EQUAL 1 AND NOT _lvrs_first_candidate STREQUAL "")
        set(${out_var} "${_lvrs_first_candidate}" PARENT_SCOPE)
        return()
    endif()

    if(_lvrs_candidate_count GREATER 1 AND NOT _lvrs_first_candidate STREQUAL "")
        message(STATUS "LVRS bootstrap: '${app_target}.xcodeproj' was not found; using '${_lvrs_first_candidate}'.")
        set(${out_var} "${_lvrs_first_candidate}" PARENT_SCOPE)
        return()
    endif()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

function(_lvrs_bootstrap_resolve_ios_simulator requested_name out_var)
    set(_lvrs_resolved_name "${requested_name}")

    find_program(_lvrs_xcrun xcrun)
    if(NOT _lvrs_xcrun)
        set(${out_var} "${_lvrs_resolved_name}" PARENT_SCOPE)
        return()
    endif()

    execute_process(
        COMMAND "${_lvrs_xcrun}" simctl list devices available
        RESULT_VARIABLE _lvrs_list_result
        OUTPUT_VARIABLE _lvrs_list_output
        ERROR_QUIET
    )
    if(NOT _lvrs_list_result EQUAL 0)
        set(${out_var} "${_lvrs_resolved_name}" PARENT_SCOPE)
        return()
    endif()

    string(REPLACE "\n" ";" _lvrs_lines "${_lvrs_list_output}")
    set(_lvrs_requested_found FALSE)
    set(_lvrs_first_booted "")
    set(_lvrs_first_iphone "")
    set(_lvrs_first_any "")

    foreach(_lvrs_line IN LISTS _lvrs_lines)
        string(STRIP "${_lvrs_line}" _lvrs_line)
        if(_lvrs_line STREQUAL "" OR _lvrs_line MATCHES "^--")
            continue()
        endif()
        if(NOT _lvrs_line MATCHES "^(.+) \\(([0-9A-Fa-f-]+)\\) \\((Booted|Shutdown)\\)$")
            continue()
        endif()

        set(_lvrs_device_name "${CMAKE_MATCH_1}")
        set(_lvrs_device_state "${CMAKE_MATCH_3}")
        string(STRIP "${_lvrs_device_name}" _lvrs_device_name)

        if(_lvrs_first_any STREQUAL "")
            set(_lvrs_first_any "${_lvrs_device_name}")
        endif()
        if(_lvrs_first_iphone STREQUAL "" AND _lvrs_device_name MATCHES "^iPhone")
            set(_lvrs_first_iphone "${_lvrs_device_name}")
        endif()
        if(_lvrs_first_booted STREQUAL "" AND _lvrs_device_state STREQUAL "Booted")
            set(_lvrs_first_booted "${_lvrs_device_name}")
        endif()
        if(_lvrs_device_name STREQUAL "${requested_name}")
            set(_lvrs_requested_found TRUE)
        endif()
    endforeach()

    if(NOT _lvrs_requested_found)
        if(NOT _lvrs_first_booted STREQUAL "")
            set(_lvrs_resolved_name "${_lvrs_first_booted}")
        elseif(NOT _lvrs_first_iphone STREQUAL "")
            set(_lvrs_resolved_name "${_lvrs_first_iphone}")
        elseif(NOT _lvrs_first_any STREQUAL "")
            set(_lvrs_resolved_name "${_lvrs_first_any}")
        endif()

        if(NOT _lvrs_resolved_name STREQUAL "${requested_name}")
            message(STATUS "LVRS bootstrap: requested iOS simulator '${requested_name}' is unavailable; using '${_lvrs_resolved_name}'.")
        endif()
    endif()

    set(${out_var} "${_lvrs_resolved_name}" PARENT_SCOPE)
endfunction()

function(_lvrs_bootstrap_find_android_deployment_settings build_dir app_target out_var)
    set(_lvrs_matches)
    file(GLOB_RECURSE _lvrs_candidates
        "${build_dir}/*deployment-settings*.json"
        "${build_dir}/*deployment_settings*.json"
        "${build_dir}/android-*.json"
        "${build_dir}/android*.json"
    )

    foreach(_lvrs_candidate IN LISTS _lvrs_candidates)
        if(IS_DIRECTORY "${_lvrs_candidate}")
            continue()
        endif()
        get_filename_component(_lvrs_name "${_lvrs_candidate}" NAME)
        string(TOLOWER "${_lvrs_name}" _lvrs_name_lower)
        if(_lvrs_name_lower MATCHES "deployment[-_]?settings")
            if(_lvrs_name STREQUAL "${app_target}-deployment-settings.json"
               OR _lvrs_name STREQUAL "android-${app_target}-deployment-settings.json")
                set(${out_var} "${_lvrs_candidate}" PARENT_SCOPE)
                return()
            endif()
            list(APPEND _lvrs_matches "${_lvrs_candidate}")
        endif()
    endforeach()

    if(NOT _lvrs_matches)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    list(SORT _lvrs_matches)
    list(GET _lvrs_matches 0 _lvrs_selected)
    set(${out_var} "${_lvrs_selected}" PARENT_SCOPE)
endfunction()

function(_lvrs_bootstrap_detect_androiddeployqt out_var)
    if(DEFINED LVRS_BOOTSTRAP_ANDROIDDEPLOYQT
       AND NOT LVRS_BOOTSTRAP_ANDROIDDEPLOYQT STREQUAL ""
       AND EXISTS "${LVRS_BOOTSTRAP_ANDROIDDEPLOYQT}")
        set(${out_var} "${LVRS_BOOTSTRAP_ANDROIDDEPLOYQT}" PARENT_SCOPE)
        return()
    endif()

    set(_lvrs_hints)
    if(DEFINED LVRS_BOOTSTRAP_QT_HOST_PREFIX AND NOT LVRS_BOOTSTRAP_QT_HOST_PREFIX STREQUAL "")
        list(APPEND _lvrs_hints "${LVRS_BOOTSTRAP_QT_HOST_PREFIX}/bin")
    endif()
    if(DEFINED LVRS_BOOTSTRAP_PREFIX_PATH AND NOT LVRS_BOOTSTRAP_PREFIX_PATH STREQUAL "")
        foreach(_lvrs_prefix IN LISTS LVRS_BOOTSTRAP_PREFIX_PATH)
            if(NOT _lvrs_prefix STREQUAL "")
                list(APPEND _lvrs_hints "${_lvrs_prefix}/bin")
            endif()
        endforeach()
    endif()

    find_program(_lvrs_androiddeployqt
        NAMES androiddeployqt androiddeployqt6
        HINTS ${_lvrs_hints}
    )
    set(${out_var} "${_lvrs_androiddeployqt}" PARENT_SCOPE)
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

function(_lvrs_bootstrap_find_android_apk build_dir out_var)
    set(_lvrs_matches)
    file(GLOB_RECURSE _lvrs_candidates "${build_dir}/*.apk")

    foreach(_lvrs_candidate IN LISTS _lvrs_candidates)
        if(IS_DIRECTORY "${_lvrs_candidate}")
            continue()
        endif()
        list(APPEND _lvrs_matches "${_lvrs_candidate}")
    endforeach()

    if(NOT _lvrs_matches)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    foreach(_lvrs_candidate IN LISTS _lvrs_matches)
        if(_lvrs_candidate MATCHES "/debug/" OR _lvrs_candidate MATCHES "-debug\\.apk$")
            set(${out_var} "${_lvrs_candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    list(SORT _lvrs_matches)
    list(GET _lvrs_matches 0 _lvrs_selected)
    set(${out_var} "${_lvrs_selected}" PARENT_SCOPE)
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
if(NOT DEFINED LVRS_BOOTSTRAP_APP_TARGET OR LVRS_BOOTSTRAP_APP_TARGET STREQUAL "")
    _lvrs_bootstrap_fail("LVRS_BOOTSTRAP_APP_TARGET is required.")
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
if(NOT DEFINED LVRS_BOOTSTRAP_IOS_SIMULATOR_NAME OR LVRS_BOOTSTRAP_IOS_SIMULATOR_NAME STREQUAL "")
    set(LVRS_BOOTSTRAP_IOS_SIMULATOR_NAME "iPhone 17 Pro")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_ANDROID_SERIAL)
    set(LVRS_BOOTSTRAP_ANDROID_SERIAL "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_QT_HOST_PREFIX)
    set(LVRS_BOOTSTRAP_QT_HOST_PREFIX "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT)
    if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "ios")
        set(LVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT ON)
    else()
        set(LVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT OFF)
    endif()
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT)
    if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "android")
        set(LVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT ON)
    else()
        set(LVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT OFF)
    endif()
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_ANDROID_STUDIO_PROJECT_DIR)
    set(LVRS_BOOTSTRAP_ANDROID_STUDIO_PROJECT_DIR "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_ANDROIDDEPLOYQT)
    set(LVRS_BOOTSTRAP_ANDROIDDEPLOYQT "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_LVRS_DIR)
    set(LVRS_BOOTSTRAP_LVRS_DIR "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_FIND_PACKAGE_NO_PACKAGE_REGISTRY)
    set(LVRS_BOOTSTRAP_FIND_PACKAGE_NO_PACKAGE_REGISTRY "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_FIND_USE_PACKAGE_REGISTRY)
    set(LVRS_BOOTSTRAP_FIND_USE_PACKAGE_REGISTRY "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_HOST_BUILD_DIR)
    set(LVRS_BOOTSTRAP_HOST_BUILD_DIR "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_ANDROID_SDK_ROOT)
    set(LVRS_BOOTSTRAP_ANDROID_SDK_ROOT "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_ANDROID_NDK)
    set(LVRS_BOOTSTRAP_ANDROID_NDK "")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES)
    set(LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES "OFF")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_LVRS_BUILD_TESTS)
    set(LVRS_BOOTSTRAP_LVRS_BUILD_TESTS "OFF")
endif()
if(NOT DEFINED LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS)
    set(LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS "")
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
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        _lvrs_bootstrap_filter_apple_x86_architectures(
            "${LVRS_BOOTSTRAP_IOS_ARCHITECTURES}"
            _lvrs_ios_architectures_filtered
        )
        if(NOT LVRS_BOOTSTRAP_IOS_ARCHITECTURES STREQUAL "${_lvrs_ios_architectures_filtered}")
            message(STATUS "LVRS bootstrap: removed Apple x86 iOS architectures.")
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
        string(TOLOWER "${LVRS_BOOTSTRAP_APP_TARGET}" _lvrs_bundle_suffix)
        string(REGEX REPLACE "[^a-z0-9.-]" "-" _lvrs_bundle_suffix "${_lvrs_bundle_suffix}")
        string(REGEX REPLACE "[-.]{2,}" "." _lvrs_bundle_suffix "${_lvrs_bundle_suffix}")
        string(REGEX REPLACE "^[.-]+" "" _lvrs_bundle_suffix "${_lvrs_bundle_suffix}")
        string(REGEX REPLACE "[.-]+$" "" _lvrs_bundle_suffix "${_lvrs_bundle_suffix}")
        if(_lvrs_bundle_suffix STREQUAL "")
            set(_lvrs_bundle_suffix "app")
        endif()
        set(LVRS_BOOTSTRAP_IOS_BUNDLE_IDENTIFIER "com.lvrs.${_lvrs_bundle_suffix}")
    endif()
endif()

if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "wasm")
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

if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "android")
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
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "LVRS_DIR" "${LVRS_BOOTSTRAP_LVRS_DIR}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY" "${LVRS_BOOTSTRAP_FIND_PACKAGE_NO_PACKAGE_REGISTRY}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "CMAKE_FIND_USE_PACKAGE_REGISTRY" "${LVRS_BOOTSTRAP_FIND_USE_PACKAGE_REGISTRY}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "LVRS_BUILD_EXAMPLES" "${LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "LVRS_BUILD_TESTS" "${LVRS_BOOTSTRAP_LVRS_BUILD_TESTS}")
_lvrs_bootstrap_append_cache_arg(_lvrs_configure_cmd "LVRS_BUILD_SHARED_LIBS" "${LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS}")

message(STATUS "LVRS bootstrap: configure '${LVRS_BOOTSTRAP_PLATFORM}' -> ${LVRS_BOOTSTRAP_BINARY_DIR}")
execute_process(
    COMMAND ${_lvrs_configure_cmd}
    RESULT_VARIABLE _lvrs_configure_result
    COMMAND_ECHO STDOUT
)
if(NOT _lvrs_configure_result EQUAL 0)
    _lvrs_bootstrap_fail("configure failed for platform '${LVRS_BOOTSTRAP_PLATFORM}' (exit=${_lvrs_configure_result}).")
endif()

if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "ios" AND LVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT)
    _lvrs_bootstrap_find_xcode_project(
        "${LVRS_BOOTSTRAP_BINARY_DIR}"
        "${LVRS_BOOTSTRAP_APP_TARGET}"
        _lvrs_xcode_project
    )
    if(_lvrs_xcode_project STREQUAL "")
        _lvrs_bootstrap_fail("Xcode project (*.xcodeproj) was not generated for iOS bootstrap build.")
    endif()
    message(STATUS "LVRS bootstrap: Xcode project ready -> ${_lvrs_xcode_project}")
endif()

set(_lvrs_build_cmd
    "${CMAKE_COMMAND}"
    --build "${LVRS_BOOTSTRAP_BINARY_DIR}"
    --target "${LVRS_BOOTSTRAP_APP_TARGET}"
)
if(NOT LVRS_BOOTSTRAP_BUILD_TYPE STREQUAL "")
    list(APPEND _lvrs_build_cmd --config "${LVRS_BOOTSTRAP_BUILD_TYPE}")
endif()

message(STATUS "LVRS bootstrap: build '${LVRS_BOOTSTRAP_APP_TARGET}' for '${LVRS_BOOTSTRAP_PLATFORM}'")
execute_process(
    COMMAND ${_lvrs_build_cmd}
    RESULT_VARIABLE _lvrs_build_result
    COMMAND_ECHO STDOUT
)
if(NOT _lvrs_build_result EQUAL 0)
    _lvrs_bootstrap_fail("build failed for platform '${LVRS_BOOTSTRAP_PLATFORM}' (exit=${_lvrs_build_result}).")
endif()

if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "macos"
   OR LVRS_BOOTSTRAP_PLATFORM STREQUAL "linux"
   OR LVRS_BOOTSTRAP_PLATFORM STREQUAL "windows")
    _lvrs_bootstrap_find_desktop_executable(
        "${LVRS_BOOTSTRAP_BINARY_DIR}"
        "${LVRS_BOOTSTRAP_HOST_BUILD_DIR}"
        "${LVRS_BOOTSTRAP_APP_TARGET}"
        "${LVRS_BOOTSTRAP_PLATFORM}"
        _lvrs_executable
    )
    if(_lvrs_executable STREQUAL "")
        _lvrs_bootstrap_fail("desktop executable artifact was not found after build.")
    endif()
    message(STATUS "LVRS bootstrap: desktop artifact ready -> ${_lvrs_executable}")
    return()
endif()

if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "ios")
    _lvrs_bootstrap_find_ios_app_bundle(
        "${LVRS_BOOTSTRAP_BINARY_DIR}"
        "${LVRS_BOOTSTRAP_APP_TARGET}"
        _lvrs_ios_app_bundle
    )
    if(_lvrs_ios_app_bundle STREQUAL "")
        _lvrs_bootstrap_fail("iOS Simulator app bundle (*.app) was not found after build.")
    endif()

    find_program(_lvrs_xcrun xcrun)
    if(NOT _lvrs_xcrun)
        _lvrs_bootstrap_fail("xcrun is required for iOS simulator installation.")
    endif()

    _lvrs_bootstrap_resolve_ios_simulator(
        "${LVRS_BOOTSTRAP_IOS_SIMULATOR_NAME}"
        _lvrs_ios_simulator_name_resolved
    )

    execute_process(
        COMMAND "${_lvrs_xcrun}" simctl boot "${_lvrs_ios_simulator_name_resolved}"
        RESULT_VARIABLE _lvrs_boot_result
        OUTPUT_VARIABLE _lvrs_boot_output
        ERROR_VARIABLE _lvrs_boot_error
    )
    if(NOT _lvrs_boot_result EQUAL 0)
        if(NOT _lvrs_boot_error MATCHES "Booted")
            message(STATUS "LVRS bootstrap: simctl boot output: ${_lvrs_boot_output}")
            message(STATUS "LVRS bootstrap: simctl boot error: ${_lvrs_boot_error}")
        endif()
    endif()

    execute_process(
        COMMAND "${_lvrs_xcrun}" simctl bootstatus "${_lvrs_ios_simulator_name_resolved}" -b
        RESULT_VARIABLE _lvrs_bootstatus_result
        COMMAND_ECHO STDOUT
    )
    if(NOT _lvrs_bootstatus_result EQUAL 0)
        _lvrs_bootstrap_fail("failed to reach booted iOS simulator state for '${_lvrs_ios_simulator_name_resolved}'.")
    endif()

    execute_process(
        COMMAND "${_lvrs_xcrun}" simctl install booted "${_lvrs_ios_app_bundle}"
        RESULT_VARIABLE _lvrs_install_result
        COMMAND_ECHO STDOUT
    )
    if(NOT _lvrs_install_result EQUAL 0)
        _lvrs_bootstrap_fail("failed to install iOS app bundle to simulator.")
    endif()

    message(STATUS "LVRS bootstrap: iOS simulator install completed -> ${_lvrs_ios_app_bundle}")
    return()
endif()

if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "android")
    if(LVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT)
        _lvrs_bootstrap_find_android_deployment_settings(
            "${LVRS_BOOTSTRAP_BINARY_DIR}"
            "${LVRS_BOOTSTRAP_APP_TARGET}"
            _lvrs_android_deployment_settings
        )
        if(_lvrs_android_deployment_settings STREQUAL "")
            _lvrs_bootstrap_fail("Android deployment settings JSON was not found for Android Studio project generation.")
        endif()

        _lvrs_bootstrap_detect_androiddeployqt(_lvrs_androiddeployqt)
        if(_lvrs_androiddeployqt STREQUAL "")
            _lvrs_bootstrap_fail("androiddeployqt tool was not found for Android Studio project generation.")
        endif()

        if(LVRS_BOOTSTRAP_ANDROID_STUDIO_PROJECT_DIR STREQUAL "")
            set(_lvrs_android_studio_dir "${LVRS_BOOTSTRAP_BINARY_DIR}/android-studio")
        else()
            set(_lvrs_android_studio_dir "${LVRS_BOOTSTRAP_ANDROID_STUDIO_PROJECT_DIR}")
        endif()
        file(MAKE_DIRECTORY "${_lvrs_android_studio_dir}")

        set(_lvrs_android_studio_cmd
            "${_lvrs_androiddeployqt}"
            --input "${_lvrs_android_deployment_settings}"
            --output "${_lvrs_android_studio_dir}"
            --aux-mode
            --verbose
        )

        execute_process(
            COMMAND ${_lvrs_android_studio_cmd}
            RESULT_VARIABLE _lvrs_android_studio_result
            COMMAND_ECHO STDOUT
        )
        if(NOT _lvrs_android_studio_result EQUAL 0)
            _lvrs_bootstrap_fail("failed to generate Android Studio project with androiddeployqt.")
        endif()

        message(STATUS "LVRS bootstrap: Android Studio project ready -> ${_lvrs_android_studio_dir}")
    endif()

    _lvrs_bootstrap_find_android_apk("${LVRS_BOOTSTRAP_BINARY_DIR}" _lvrs_android_apk)
    if(_lvrs_android_apk STREQUAL "")
        _lvrs_bootstrap_fail("Android APK artifact was not found after build.")
    endif()

    find_program(_lvrs_adb adb)
    if(NOT _lvrs_adb)
        _lvrs_bootstrap_fail("adb is required for Android emulator installation.")
    endif()

    if(NOT LVRS_BOOTSTRAP_ANDROID_SERIAL STREQUAL "")
        set(_lvrs_android_install_cmd
            "${_lvrs_adb}" -s "${LVRS_BOOTSTRAP_ANDROID_SERIAL}" install -r "${_lvrs_android_apk}")
    else()
        set(_lvrs_android_install_cmd
            "${_lvrs_adb}" install -r "${_lvrs_android_apk}")
    endif()

    execute_process(
        COMMAND ${_lvrs_android_install_cmd}
        RESULT_VARIABLE _lvrs_android_install_result
        COMMAND_ECHO STDOUT
    )
    if(NOT _lvrs_android_install_result EQUAL 0)
        _lvrs_bootstrap_fail("failed to install APK to Android emulator/device.")
    endif()

    message(STATUS "LVRS bootstrap: Android install completed -> ${_lvrs_android_apk}")
    return()
endif()

if(LVRS_BOOTSTRAP_PLATFORM STREQUAL "wasm")
    _lvrs_bootstrap_find_wasm_entry_html(
        "${LVRS_BOOTSTRAP_BINARY_DIR}"
        "${LVRS_BOOTSTRAP_APP_TARGET}"
        _lvrs_wasm_entry_html
    )
    if(_lvrs_wasm_entry_html STREQUAL "")
        _lvrs_bootstrap_fail("WASM HTML entry artifact (*.html) was not found after build.")
    endif()

    get_filename_component(_lvrs_wasm_artifact_dir "${_lvrs_wasm_entry_html}" DIRECTORY)
    file(RELATIVE_PATH _lvrs_wasm_entry_relative
        "${LVRS_BOOTSTRAP_BINARY_DIR}"
        "${_lvrs_wasm_entry_html}"
    )
    set(_lvrs_wasm_metadata_file "${LVRS_BOOTSTRAP_BINARY_DIR}/LVRSWasmArtifact.cmake")
    file(WRITE "${_lvrs_wasm_metadata_file}"
        "set(LVRS_WASM_ENTRY_HTML \"${_lvrs_wasm_entry_html}\")\n"
        "set(LVRS_WASM_ENTRY_RELATIVE \"${_lvrs_wasm_entry_relative}\")\n"
        "set(LVRS_WASM_ARTIFACT_DIR \"${_lvrs_wasm_artifact_dir}\")\n"
    )
    message(STATUS "LVRS bootstrap: WASM artifact ready -> ${_lvrs_wasm_entry_html}")
    message(STATUS "LVRS bootstrap: WASM metadata -> ${_lvrs_wasm_metadata_file}")
    message(STATUS "LVRS bootstrap: serve '${_lvrs_wasm_artifact_dir}' with a static HTTP server.")
    return()
endif()

_lvrs_bootstrap_fail("unsupported platform '${LVRS_BOOTSTRAP_PLATFORM}'.")
