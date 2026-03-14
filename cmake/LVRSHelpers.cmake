include_guard(GLOBAL)
include(CMakeParseArguments)

function(_lvrs_internal_append_unique_qml_import_path target path_value)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "lvrs_configure_qml_app() target not found: ${target}")
    endif()

    if(path_value STREQUAL "" OR NOT IS_DIRECTORY "${path_value}")
        return()
    endif()

    get_target_property(_lvrs_existing_import_paths "${target}" QT_QML_IMPORT_PATH)
    if(NOT _lvrs_existing_import_paths OR _lvrs_existing_import_paths STREQUAL "QT_QML_IMPORT_PATH-NOTFOUND")
        set(_lvrs_existing_import_paths "")
    endif()

    set(_lvrs_paths "${_lvrs_existing_import_paths}")
    list(APPEND _lvrs_paths "${path_value}")
    list(REMOVE_DUPLICATES _lvrs_paths)
    set_property(TARGET "${target}" PROPERTY QT_QML_IMPORT_PATH "${_lvrs_paths}")
endfunction()

function(_lvrs_internal_append_unique_target_list_property target property_name value)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "_lvrs_internal_append_unique_target_list_property() target not found: ${target}")
    endif()

    if(value STREQUAL "")
        return()
    endif()

    get_target_property(_lvrs_existing_values "${target}" "${property_name}")
    if(NOT _lvrs_existing_values OR _lvrs_existing_values STREQUAL "${property_name}-NOTFOUND")
        set(_lvrs_existing_values "")
    endif()

    set(_lvrs_values "${_lvrs_existing_values}")
    list(APPEND _lvrs_values "${value}")
    list(REMOVE_DUPLICATES _lvrs_values)
    set_property(TARGET "${target}" PROPERTY "${property_name}" "${_lvrs_values}")
endfunction()

function(_lvrs_internal_apply_safe_default_output_dirs target)
    get_target_property(_lvrs_runtime_output_dir "${target}" RUNTIME_OUTPUT_DIRECTORY)
    if(NOT _lvrs_runtime_output_dir OR _lvrs_runtime_output_dir STREQUAL "RUNTIME_OUTPUT_DIRECTORY-NOTFOUND")
        # Prevent executable/output name collisions with qt_add_qml_module default OUTPUT_DIRECTORY.
        set_property(TARGET "${target}" PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
    endif()
endfunction()

function(_lvrs_internal_example_source_snapshot_runtime_rpath runtime_platform out_var)
    set(_lvrs_runtime_rpath "")
    if(runtime_platform STREQUAL "macos")
        set(_lvrs_runtime_rpath "@loader_path/../../../../platforms/macos/lib")
    elseif(runtime_platform STREQUAL "linux")
        set(_lvrs_runtime_rpath "$ORIGIN/../../../../platforms/linux/lib")
    endif()

    set(${out_var} "${_lvrs_runtime_rpath}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_example_runtime_output_dir out_var)
    if(NOT DEFINED LVRS_EXAMPLE_RUNTIME_OUTPUT_DIR OR LVRS_EXAMPLE_RUNTIME_OUTPUT_DIR STREQUAL "")
        set(_lvrs_example_runtime_output_dir "${CMAKE_CURRENT_BINARY_DIR}/bin")
    else()
        set(_lvrs_example_runtime_output_dir "${LVRS_EXAMPLE_RUNTIME_OUTPUT_DIR}")
    endif()

    set(${out_var} "${_lvrs_example_runtime_output_dir}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_configure_example_source_snapshot_runtime target runtime_platform)
    _lvrs_internal_example_source_snapshot_runtime_rpath(
        "${runtime_platform}"
        _lvrs_runtime_rpath
    )
    if(_lvrs_runtime_rpath STREQUAL "")
        return()
    endif()

    _lvrs_internal_append_unique_target_list_property(
        "${target}"
        BUILD_RPATH
        "${_lvrs_runtime_rpath}"
    )
endfunction()

function(_lvrs_internal_maybe_link_static_lvrs_plugin target)
    set(_lvrs_plugin_target "")
    if(TARGET LVRSCoreplugin)
        set(_lvrs_plugin_target LVRSCoreplugin)
    elseif(TARGET LVRS::LVRSCoreplugin)
        set(_lvrs_plugin_target LVRS::LVRSCoreplugin)
    endif()

    if(_lvrs_plugin_target STREQUAL "")
        return()
    endif()

    set(_lvrs_is_static_plugin FALSE)
    get_target_property(_lvrs_plugin_type "${_lvrs_plugin_target}" TYPE)
    if(_lvrs_plugin_type STREQUAL "STATIC_LIBRARY")
        set(_lvrs_is_static_plugin TRUE)
    endif()

    if(NOT _lvrs_is_static_plugin)
        get_target_property(_lvrs_plugin_location "${_lvrs_plugin_target}" IMPORTED_LOCATION)
        if(_lvrs_plugin_location
           AND _lvrs_plugin_location MATCHES "\\${CMAKE_STATIC_LIBRARY_SUFFIX}$")
            set(_lvrs_is_static_plugin TRUE)
        endif()
    endif()

    if(_lvrs_is_static_plugin)
        target_link_libraries("${target}" PRIVATE "${_lvrs_plugin_target}")

        get_target_property(_lvrs_plugin_import_injected "${target}" _LVRS_STATIC_PLUGIN_IMPORT_INJECTED)
        if(NOT _lvrs_plugin_import_injected)
            set(_lvrs_plugin_import_source "${CMAKE_CURRENT_BINARY_DIR}/${target}_lvrs_plugin_import.cpp")
            file(WRITE "${_lvrs_plugin_import_source}" "#include <QtPlugin>\nQ_IMPORT_PLUGIN(LVRSPlugin)\n")
            target_sources("${target}" PRIVATE "${_lvrs_plugin_import_source}")
            set_property(TARGET "${target}" PROPERTY _LVRS_STATIC_PLUGIN_IMPORT_INJECTED TRUE)
        endif()
    endif()
endfunction()

function(_lvrs_internal_target_supports_link_options target out_var)
    get_target_property(_lvrs_target_type "${target}" TYPE)
    if(_lvrs_target_type STREQUAL "EXECUTABLE"
       OR _lvrs_target_type STREQUAL "SHARED_LIBRARY"
       OR _lvrs_target_type STREQUAL "MODULE_LIBRARY")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(lvrs_apply_platform_build_optimizations target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "lvrs_apply_platform_build_optimizations() target not found: ${target}")
    endif()

    if(DEFINED LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS
       AND NOT LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS)
        return()
    endif()

    get_target_property(_lvrs_platform_optimizations_applied
        "${target}" _LVRS_PLATFORM_OPTIMIZATIONS_APPLIED)
    if(_lvrs_platform_optimizations_applied)
        return()
    endif()

    _lvrs_internal_detect_target_runtime_platform(_lvrs_target_platform)
    _lvrs_internal_target_supports_link_options("${target}" _lvrs_supports_link_options)

    if(MSVC)
        target_compile_options("${target}" PRIVATE
            "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:/Gy>"
            "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:/Gw>"
        )

        if(_lvrs_supports_link_options AND _lvrs_target_platform STREQUAL "windows")
            target_link_options("${target}" PRIVATE
                "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:/OPT:REF>"
                "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:/OPT:ICF>"
            )
        endif()
    else()
        target_compile_options("${target}" PRIVATE
            "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:-ffunction-sections>"
            "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:-fdata-sections>"
        )

        if(_lvrs_supports_link_options)
            if(_lvrs_target_platform STREQUAL "linux"
               OR _lvrs_target_platform STREQUAL "android"
               OR _lvrs_target_platform STREQUAL "wasm")
                target_link_options("${target}" PRIVATE
                    "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:-Wl,--gc-sections>"
                )
            elseif(_lvrs_target_platform STREQUAL "macos"
                   OR _lvrs_target_platform STREQUAL "ios")
                target_link_options("${target}" PRIVATE
                    "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:-Wl,-dead_strip>"
                )
            endif()
        endif()
    endif()

    set(_lvrs_enable_ipo TRUE)
    if(DEFINED LVRS_ENABLE_IPO AND NOT LVRS_ENABLE_IPO)
        set(_lvrs_enable_ipo FALSE)
    endif()

    if(_lvrs_enable_ipo)
        get_property(_lvrs_ipo_support_checked GLOBAL PROPERTY _LVRS_IPO_SUPPORT_CHECKED)
        if(NOT _lvrs_ipo_support_checked)
            include(CheckIPOSupported)
            check_ipo_supported(RESULT _lvrs_ipo_supported OUTPUT _lvrs_ipo_output LANGUAGES CXX)
            set_property(GLOBAL PROPERTY _LVRS_IPO_SUPPORT_CHECKED TRUE)
            if(_lvrs_ipo_supported)
                set_property(GLOBAL PROPERTY _LVRS_IPO_SUPPORTED TRUE)
            else()
                set_property(GLOBAL PROPERTY _LVRS_IPO_SUPPORTED FALSE)
                set_property(GLOBAL PROPERTY _LVRS_IPO_OUTPUT "${_lvrs_ipo_output}")
            endif()
        endif()

        get_property(_lvrs_ipo_supported GLOBAL PROPERTY _LVRS_IPO_SUPPORTED)
        if(_lvrs_ipo_supported)
            set_property(TARGET "${target}" PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
            set_property(TARGET "${target}" PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
            set_property(TARGET "${target}" PROPERTY INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
        endif()
    endif()

    set_property(TARGET "${target}" PROPERTY _LVRS_PLATFORM_OPTIMIZATIONS_APPLIED TRUE)
endfunction()

function(lvrs_configure_example_target target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "lvrs_configure_example_target() target not found: ${target}")
    endif()

    _lvrs_internal_example_runtime_output_dir(_lvrs_example_runtime_output_dir)

    _lvrs_internal_detect_target_runtime_platform(_lvrs_example_target_platform)

    # Apply platform-specific executable flags so each OS keeps its native
    # packaging/runtime shape while desktop targets share a predictable bin dir.
    if(_lvrs_example_target_platform STREQUAL "windows")
        set_target_properties("${target}" PROPERTIES
            WIN32_EXECUTABLE TRUE
            RUNTIME_OUTPUT_DIRECTORY "${_lvrs_example_runtime_output_dir}"
        )
    elseif(_lvrs_example_target_platform STREQUAL "macos")
        set_target_properties("${target}" PROPERTIES
            MACOSX_BUNDLE OFF
            RUNTIME_OUTPUT_DIRECTORY "${_lvrs_example_runtime_output_dir}"
        )
    elseif(_lvrs_example_target_platform STREQUAL "linux")
        set_target_properties("${target}" PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${_lvrs_example_runtime_output_dir}"
        )
    elseif(_lvrs_example_target_platform STREQUAL "ios")
        set_target_properties("${target}" PROPERTIES
            MACOSX_BUNDLE TRUE
        )
    elseif(_lvrs_example_target_platform STREQUAL "android")
        # Keep Android packaging defaults generated by Qt.
    elseif(_lvrs_example_target_platform STREQUAL "wasm")
        # Keep Emscripten toolchain output layout.
    else()
        set_target_properties("${target}" PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${_lvrs_example_runtime_output_dir}"
        )
    endif()

    _lvrs_internal_configure_example_source_snapshot_runtime(
        "${target}"
        "${_lvrs_example_target_platform}"
    )

    lvrs_apply_platform_build_optimizations("${target}")

    if(LVRS_INSTALL_EXAMPLES)
        if(_lvrs_example_target_platform STREQUAL "ios")
            install(TARGETS "${target}"
                BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        elseif(_lvrs_example_target_platform STREQUAL "android")
            install(TARGETS "${target}"
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        else()
            install(TARGETS "${target}"
                BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            )
        endif()
    endif()
endfunction()

macro(lvrs_add_example_app)
    set(_lvrs_example_options)
    set(_lvrs_example_one_value_args TARGET URI VERSION)
    set(_lvrs_example_multi_value_args SOURCES QML_FILES RESOURCES INCLUDE_DIRS LINK_LIBRARIES)
    cmake_parse_arguments(LVRS_EXAMPLE
        "${_lvrs_example_options}"
        "${_lvrs_example_one_value_args}"
        "${_lvrs_example_multi_value_args}"
        ${ARGN}
    )

    if(NOT LVRS_EXAMPLE_TARGET)
        message(FATAL_ERROR "lvrs_add_example_app() requires TARGET")
    endif()
    if(NOT LVRS_EXAMPLE_URI)
        message(FATAL_ERROR "lvrs_add_example_app() requires URI")
    endif()
    if(NOT LVRS_EXAMPLE_SOURCES)
        message(FATAL_ERROR "lvrs_add_example_app() requires SOURCES")
    endif()
    if(NOT LVRS_EXAMPLE_QML_FILES)
        message(FATAL_ERROR "lvrs_add_example_app() requires QML_FILES")
    endif()
    if(NOT LVRS_EXAMPLE_VERSION)
        set(LVRS_EXAMPLE_VERSION "1.0")
    endif()

    if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        project("${LVRS_EXAMPLE_TARGET}" LANGUAGES CXX)
        set(CMAKE_CXX_STANDARD 20)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
        find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Svg Network)
        qt_standard_project_setup()
    endif()

    if(NOT TARGET LVRSCore)
        add_subdirectory("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." backend_build)
    endif()

    qt_add_executable("${LVRS_EXAMPLE_TARGET}"
        ${LVRS_EXAMPLE_SOURCES}
    )

    set(_lvrs_example_qml_module_args
        URI "${LVRS_EXAMPLE_URI}"
        VERSION "${LVRS_EXAMPLE_VERSION}"
        RESOURCE_PREFIX "/qt/qml"
        QML_FILES
            ${LVRS_EXAMPLE_QML_FILES}
    )
    if(LVRS_EXAMPLE_RESOURCES)
        list(APPEND _lvrs_example_qml_module_args
            RESOURCES
                ${LVRS_EXAMPLE_RESOURCES}
        )
    endif()

    qt_add_qml_module("${LVRS_EXAMPLE_TARGET}"
        ${_lvrs_example_qml_module_args}
    )

    target_link_libraries("${LVRS_EXAMPLE_TARGET}"
        PRIVATE
            Qt6::Quick
            Qt6::QuickControls2
            ${LVRS_EXAMPLE_LINK_LIBRARIES}
    )

    if(LVRS_EXAMPLE_INCLUDE_DIRS)
        target_include_directories("${LVRS_EXAMPLE_TARGET}"
            PRIVATE
                ${LVRS_EXAMPLE_INCLUDE_DIRS}
        )
    endif()

    lvrs_configure_qml_app("${LVRS_EXAMPLE_TARGET}")
    lvrs_configure_example_target("${LVRS_EXAMPLE_TARGET}")

    add_custom_target("run_${LVRS_EXAMPLE_TARGET}"
        COMMAND $<TARGET_FILE:${LVRS_EXAMPLE_TARGET}>
        DEPENDS "${LVRS_EXAMPLE_TARGET}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        USES_TERMINAL
    )
endmacro()

function(_lvrs_internal_known_runtime_platforms out_var)
    if(DEFINED LVRS_RUNTIME_PLATFORMS)
        set(${out_var} ${LVRS_RUNTIME_PLATFORMS} PARENT_SCOPE)
        return()
    endif()

    set(${out_var}
        macos
        linux
        windows
        ios
        android
        wasm
        PARENT_SCOPE
    )
endfunction()

function(_lvrs_internal_framework_bootstrap_platforms out_var)
    set(_lvrs_platforms_raw "")
    if(DEFINED LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS
       AND NOT LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS STREQUAL "")
        set(_lvrs_platforms_raw "${LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS}
           AND NOT "$ENV{LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS}" STREQUAL "")
        set(_lvrs_platforms_raw "$ENV{LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS}")
    endif()

    if(_lvrs_platforms_raw STREQUAL "")
        # Framework bootstrap defaults to all supported runtime platforms.
        # Individual targets can still be skipped later when a matching Qt kit
        # is not available on the current machine.
        _lvrs_internal_known_runtime_platforms(_lvrs_platforms)
    else()
        string(REPLACE "," ";" _lvrs_platforms "${_lvrs_platforms_raw}")
    endif()

    set(_lvrs_filtered_platforms "")
    foreach(_lvrs_platform IN LISTS _lvrs_platforms)
        string(STRIP "${_lvrs_platform}" _lvrs_platform)
        if(_lvrs_platform STREQUAL "")
            continue()
        endif()
        list(APPEND _lvrs_filtered_platforms "${_lvrs_platform}")
    endforeach()
    list(REMOVE_DUPLICATES _lvrs_filtered_platforms)

    if(NOT _lvrs_filtered_platforms)
        _lvrs_internal_known_runtime_platforms(_lvrs_filtered_platforms)
    endif()

    set(${out_var} ${_lvrs_filtered_platforms} PARENT_SCOPE)
endfunction()

function(_lvrs_internal_runtime_platform_from_system_name system_name osx_sysroot out_var)
    set(_lvrs_platform unknown)

    if(system_name STREQUAL "Android")
        set(_lvrs_platform android)
    elseif(system_name STREQUAL "iOS")
        set(_lvrs_platform ios)
    elseif(system_name STREQUAL "Emscripten")
        set(_lvrs_platform wasm)
    elseif(system_name STREQUAL "Darwin")
        if(osx_sysroot MATCHES "iphone")
            set(_lvrs_platform ios)
        else()
            set(_lvrs_platform macos)
        endif()
    elseif(system_name STREQUAL "Windows")
        set(_lvrs_platform windows)
    elseif(system_name STREQUAL "Linux")
        set(_lvrs_platform linux)
    endif()

    set(${out_var} "${_lvrs_platform}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_detect_host_runtime_platform out_var)
    _lvrs_internal_runtime_platform_from_system_name("${CMAKE_HOST_SYSTEM_NAME}" "" _lvrs_host_platform)
    set(${out_var} "${_lvrs_host_platform}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_detect_target_runtime_platform out_var)
    _lvrs_internal_runtime_platform_from_system_name("${CMAKE_SYSTEM_NAME}" "${CMAKE_OSX_SYSROOT}" _lvrs_target_platform)
    set(${out_var} "${_lvrs_target_platform}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_platform_to_cmake_system_name platform out_var)
    if(platform STREQUAL "macos")
        set(_lvrs_system_name "Darwin")
    elseif(platform STREQUAL "linux")
        set(_lvrs_system_name "Linux")
    elseif(platform STREQUAL "windows")
        set(_lvrs_system_name "Windows")
    elseif(platform STREQUAL "ios")
        set(_lvrs_system_name "iOS")
    elseif(platform STREQUAL "android")
        set(_lvrs_system_name "Android")
    elseif(platform STREQUAL "wasm")
        set(_lvrs_system_name "Emscripten")
    else()
        set(_lvrs_system_name "Unknown")
    endif()

    set(${out_var} "${_lvrs_system_name}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_platform_supports_direct_run platform out_var)
    if(platform STREQUAL "macos" OR platform STREQUAL "linux" OR platform STREQUAL "windows")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(_lvrs_internal_escape_list_for_cache in_value out_var)
    set(_lvrs_value "${in_value}")
    string(REPLACE ";" "\\;" _lvrs_value "${_lvrs_value}")
    set(${out_var} "${_lvrs_value}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_platform_qt_dir_candidates platform out_var)
    if(platform STREQUAL "macos")
        set(_lvrs_candidates macos)
    elseif(platform STREQUAL "linux")
        set(_lvrs_candidates gcc_64 linux)
    elseif(platform STREQUAL "windows")
        set(_lvrs_candidates msvc2022_64 msvc2019_64 mingw_64 windows)
    elseif(platform STREQUAL "ios")
        set(_lvrs_candidates ios)
    elseif(platform STREQUAL "android")
        set(_lvrs_candidates android_arm64_v8a android)
    elseif(platform STREQUAL "wasm")
        # Prefer multithread kits first because singlethread kits can omit APIs
        # such as QThreadPool that LVRS currently uses.
        set(_lvrs_candidates wasm_multithread wasm_singlethread wasm_32 wasm)
    else()
        set(_lvrs_candidates)
    endif()

    set(${out_var} ${_lvrs_candidates} PARENT_SCOPE)
endfunction()

function(_lvrs_internal_resolve_qt_prefix_candidate candidate out_var)
    set(_lvrs_resolved_prefix "")
    if(candidate STREQUAL "")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    if(IS_DIRECTORY "${candidate}" AND EXISTS "${candidate}/lib/cmake/Qt6/Qt6Config.cmake")
        set(_lvrs_resolved_prefix "${candidate}")
    elseif(IS_DIRECTORY "${candidate}" AND EXISTS "${candidate}/Qt6Config.cmake")
        get_filename_component(_lvrs_resolved_prefix "${candidate}/../../.." ABSOLUTE)
    elseif(candidate MATCHES "Qt6Config\\.cmake$" AND EXISTS "${candidate}")
        get_filename_component(_lvrs_qt6_cmake_dir "${candidate}" DIRECTORY)
        get_filename_component(_lvrs_resolved_prefix "${_lvrs_qt6_cmake_dir}/../../.." ABSOLUTE)
        unset(_lvrs_qt6_cmake_dir)
    elseif(IS_DIRECTORY "${candidate}" AND EXISTS "${candidate}/bin/qtpaths")
        set(_lvrs_resolved_prefix "${candidate}")
    endif()

    set(${out_var} "${_lvrs_resolved_prefix}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_detect_qt_prefix_for_platform platform out_var)
    string(TOUPPER "${platform}" _lvrs_upper)
    set(_lvrs_override_var "LVRS_BOOTSTRAP_QT_PREFIX_${_lvrs_upper}")
    set(_lvrs_qt_prefix "")

    if(DEFINED ${_lvrs_override_var})
        set(_lvrs_qt_prefix "${${_lvrs_override_var}}")
    endif()
    if(_lvrs_qt_prefix STREQUAL "" AND DEFINED ENV{${_lvrs_override_var}})
        set(_lvrs_qt_prefix "$ENV{${_lvrs_override_var}}")
    endif()
    if(_lvrs_qt_prefix STREQUAL "" AND DEFINED LVRS_BOOTSTRAP_QT_PREFIX)
        set(_lvrs_qt_prefix "${LVRS_BOOTSTRAP_QT_PREFIX}")
    endif()
    if(_lvrs_qt_prefix STREQUAL "" AND DEFINED ENV{LVRS_BOOTSTRAP_QT_PREFIX})
        set(_lvrs_qt_prefix "$ENV{LVRS_BOOTSTRAP_QT_PREFIX}")
    endif()

    set(_lvrs_platform_hint_vars "")
    if(platform STREQUAL "ios")
        list(APPEND _lvrs_platform_hint_vars LVRS_QT_IOS_PREFIX_HINT)
    elseif(platform STREQUAL "android")
        list(APPEND _lvrs_platform_hint_vars LVRS_QT_ANDROID_PREFIX_HINT)
    elseif(platform STREQUAL "wasm")
        list(APPEND _lvrs_platform_hint_vars LVRS_QT_WASM_PREFIX_HINT)
    endif()

    if(_lvrs_qt_prefix STREQUAL "")
        foreach(_lvrs_hint_var IN LISTS _lvrs_platform_hint_vars)
            if(DEFINED ${_lvrs_hint_var} AND NOT "${${_lvrs_hint_var}}" STREQUAL "")
                set(_lvrs_qt_prefix "${${_lvrs_hint_var}}")
                break()
            endif()
            if(DEFINED ENV{${_lvrs_hint_var}} AND NOT "$ENV{${_lvrs_hint_var}}" STREQUAL "")
                set(_lvrs_qt_prefix "$ENV{${_lvrs_hint_var}}")
                break()
            endif()
        endforeach()
    endif()

    if(NOT _lvrs_qt_prefix STREQUAL "")
        _lvrs_internal_resolve_qt_prefix_candidate("${_lvrs_qt_prefix}" _lvrs_qt_prefix_resolved)
        if(NOT _lvrs_qt_prefix_resolved STREQUAL "")
            set(${out_var} "${_lvrs_qt_prefix_resolved}" PARENT_SCOPE)
            return()
        endif()
    endif()

    if(NOT DEFINED Qt6_DIR)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    get_filename_component(_lvrs_current_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
    get_filename_component(_lvrs_qt_version_root "${_lvrs_current_qt_prefix}" DIRECTORY)

    _lvrs_internal_platform_qt_dir_candidates("${platform}" _lvrs_qt_candidates)
    foreach(_lvrs_candidate_name IN LISTS _lvrs_qt_candidates)
        set(_lvrs_candidate_prefix "${_lvrs_qt_version_root}/${_lvrs_candidate_name}")
        _lvrs_internal_resolve_qt_prefix_candidate("${_lvrs_candidate_prefix}" _lvrs_candidate_prefix_resolved)
        if(NOT _lvrs_candidate_prefix_resolved STREQUAL "")
            set(${out_var} "${_lvrs_candidate_prefix_resolved}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    if(platform STREQUAL "wasm")
        file(GLOB _lvrs_qt_platform_dirs LIST_DIRECTORIES true "${_lvrs_qt_version_root}/*")
        foreach(_lvrs_qt_platform_dir IN LISTS _lvrs_qt_platform_dirs)
            if(NOT IS_DIRECTORY "${_lvrs_qt_platform_dir}")
                continue()
            endif()
            get_filename_component(_lvrs_qt_platform_name "${_lvrs_qt_platform_dir}" NAME)
            string(TOLOWER "${_lvrs_qt_platform_name}" _lvrs_qt_platform_name_lower)
            if(NOT _lvrs_qt_platform_name_lower MATCHES "(wasm|emscripten)")
                continue()
            endif()
            _lvrs_internal_resolve_qt_prefix_candidate("${_lvrs_qt_platform_dir}" _lvrs_candidate_prefix_resolved)
            if(NOT _lvrs_candidate_prefix_resolved STREQUAL "")
                set(${out_var} "${_lvrs_candidate_prefix_resolved}" PARENT_SCOPE)
                return()
            endif()
        endforeach()
    endif()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_report_missing_bootstrap_qt_kit bootstrap_kind platform)
    string(TOUPPER "${platform}" _lvrs_platform_upper)
    message(STATUS
        "LVRS ${bootstrap_kind} bootstrap targets: skipping '${platform}' "
        "(Qt kit not found). Set LVRS_BOOTSTRAP_QT_PREFIX_${_lvrs_platform_upper} "
        "or install the matching Qt platform kit."
    )
endfunction()

function(_lvrs_internal_default_toolchain_for_qt_prefix qt_prefix out_var)
    if(qt_prefix STREQUAL "")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(_lvrs_toolchain_candidate "${qt_prefix}/lib/cmake/Qt6/qt.toolchain.cmake")
    if(EXISTS "${_lvrs_toolchain_candidate}")
        set(${out_var} "${_lvrs_toolchain_candidate}" PARENT_SCOPE)
        return()
    endif()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_bootstrap_toolchain_for_platform platform qt_prefix out_var)
    string(TOUPPER "${platform}" _lvrs_upper)
    set(_lvrs_override_var "LVRS_BOOTSTRAP_TOOLCHAIN_FILE_${_lvrs_upper}")
    set(_lvrs_toolchain "")

    if(DEFINED ${_lvrs_override_var})
        set(_lvrs_toolchain "${${_lvrs_override_var}}")
    endif()
    if(_lvrs_toolchain STREQUAL "" AND DEFINED ENV{${_lvrs_override_var}})
        set(_lvrs_toolchain "$ENV{${_lvrs_override_var}}")
    endif()
    if(_lvrs_toolchain STREQUAL "" AND DEFINED LVRS_BOOTSTRAP_TOOLCHAIN_FILE)
        set(_lvrs_toolchain "${LVRS_BOOTSTRAP_TOOLCHAIN_FILE}")
    endif()
    if(_lvrs_toolchain STREQUAL "" AND DEFINED ENV{LVRS_BOOTSTRAP_TOOLCHAIN_FILE})
        set(_lvrs_toolchain "$ENV{LVRS_BOOTSTRAP_TOOLCHAIN_FILE}")
    endif()

    if(_lvrs_toolchain STREQUAL "" AND (platform STREQUAL "ios" OR platform STREQUAL "android" OR platform STREQUAL "wasm"))
        _lvrs_internal_default_toolchain_for_qt_prefix("${qt_prefix}" _lvrs_toolchain)
    endif()

    set(${out_var} "${_lvrs_toolchain}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_bootstrap_generator_for_platform platform out_var)
    string(TOUPPER "${platform}" _lvrs_upper)
    set(_lvrs_override_var "LVRS_BOOTSTRAP_GENERATOR_${_lvrs_upper}")
    set(_lvrs_generator "")

    if(DEFINED ${_lvrs_override_var})
        set(_lvrs_generator "${${_lvrs_override_var}}")
    endif()
    if(_lvrs_generator STREQUAL "" AND DEFINED ENV{${_lvrs_override_var}})
        set(_lvrs_generator "$ENV{${_lvrs_override_var}}")
    endif()
    if(_lvrs_generator STREQUAL "" AND DEFINED LVRS_BOOTSTRAP_GENERATOR)
        set(_lvrs_generator "${LVRS_BOOTSTRAP_GENERATOR}")
    endif()
    if(_lvrs_generator STREQUAL "" AND DEFINED ENV{LVRS_BOOTSTRAP_GENERATOR})
        set(_lvrs_generator "$ENV{LVRS_BOOTSTRAP_GENERATOR}")
    endif()

    if(_lvrs_generator STREQUAL "" AND platform STREQUAL "ios")
        set(_lvrs_generator "Xcode")
    endif()

    if(_lvrs_generator STREQUAL "" AND platform STREQUAL "wasm")
        find_program(_lvrs_ninja_program NAMES ninja ninja-build)
        if(_lvrs_ninja_program)
            set(_lvrs_generator "Ninja")
        elseif(DEFINED CMAKE_GENERATOR AND NOT CMAKE_GENERATOR STREQUAL "")
            set(_lvrs_generator "${CMAKE_GENERATOR}")
        endif()
        unset(_lvrs_ninja_program)
    endif()

    if(_lvrs_generator STREQUAL "" AND DEFINED CMAKE_GENERATOR AND NOT CMAKE_GENERATOR STREQUAL "")
        set(_lvrs_generator "${CMAKE_GENERATOR}")
    endif()

    set(${out_var} "${_lvrs_generator}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_bootstrap_system_name_for_platform platform out_var)
    if(platform STREQUAL "macos")
        set(_lvrs_system_name "Darwin")
    elseif(platform STREQUAL "linux")
        set(_lvrs_system_name "Linux")
    elseif(platform STREQUAL "windows")
        set(_lvrs_system_name "Windows")
    elseif(platform STREQUAL "ios")
        set(_lvrs_system_name "iOS")
    elseif(platform STREQUAL "android")
        set(_lvrs_system_name "Android")
    elseif(platform STREQUAL "wasm")
        set(_lvrs_system_name "Emscripten")
    else()
        set(_lvrs_system_name "Unknown")
    endif()

    set(${out_var} "${_lvrs_system_name}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_bootstrap_osx_sysroot_for_platform platform out_var)
    string(TOUPPER "${platform}" _lvrs_upper)
    set(_lvrs_override_var "LVRS_BOOTSTRAP_OSX_SYSROOT_${_lvrs_upper}")
    set(_lvrs_sysroot "")

    if(DEFINED ${_lvrs_override_var})
        set(_lvrs_sysroot "${${_lvrs_override_var}}")
    endif()
    if(_lvrs_sysroot STREQUAL "" AND DEFINED ENV{${_lvrs_override_var}})
        set(_lvrs_sysroot "$ENV{${_lvrs_override_var}}")
    endif()

    if(_lvrs_sysroot STREQUAL "" AND platform STREQUAL "ios")
        set(_lvrs_sysroot "iphonesimulator")
    endif()

    set(${out_var} "${_lvrs_sysroot}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_bootstrap_android_abi_for_platform platform out_var)
    if(NOT platform STREQUAL "android")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    if(DEFINED LVRS_BOOTSTRAP_ANDROID_ABI)
        set(${out_var} "${LVRS_BOOTSTRAP_ANDROID_ABI}" PARENT_SCOPE)
        return()
    endif()
    if(DEFINED ENV{LVRS_BOOTSTRAP_ANDROID_ABI})
        set(${out_var} "$ENV{LVRS_BOOTSTRAP_ANDROID_ABI}" PARENT_SCOPE)
        return()
    endif()

    if(DEFINED CMAKE_ANDROID_ARCH_ABI AND NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "")
        set(${out_var} "${CMAKE_ANDROID_ARCH_ABI}" PARENT_SCOPE)
        return()
    endif()

    set(${out_var} "" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_filter_apple_x86_architectures architectures out_var)
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

function(_lvrs_internal_bootstrap_build_type out_var)
    if(DEFINED LVRS_BOOTSTRAP_BUILD_TYPE AND NOT LVRS_BOOTSTRAP_BUILD_TYPE STREQUAL "")
        set(${out_var} "${LVRS_BOOTSTRAP_BUILD_TYPE}" PARENT_SCOPE)
        return()
    endif()
    if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL "")
        set(${out_var} "${CMAKE_BUILD_TYPE}" PARENT_SCOPE)
        return()
    endif()
    set(${out_var} "Release" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_detect_qt_host_prefix out_var)
    set(_lvrs_qt_host_prefix "")
    if(DEFINED LVRS_BOOTSTRAP_QT_HOST_PREFIX AND NOT LVRS_BOOTSTRAP_QT_HOST_PREFIX STREQUAL "")
        set(_lvrs_qt_host_prefix "${LVRS_BOOTSTRAP_QT_HOST_PREFIX}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_QT_HOST_PREFIX} AND NOT "$ENV{LVRS_BOOTSTRAP_QT_HOST_PREFIX}" STREQUAL "")
        set(_lvrs_qt_host_prefix "$ENV{LVRS_BOOTSTRAP_QT_HOST_PREFIX}")
    elseif(DEFINED Qt6_DIR)
        get_filename_component(_lvrs_qt_host_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
    endif()

    set(${out_var} "${_lvrs_qt_host_prefix}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_resolve_install_root_from_lvrs_hint hint out_var)
    set(_lvrs_install_root "")
    if(hint STREQUAL "")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    get_filename_component(_lvrs_hint_abs "${hint}" ABSOLUTE)
    file(TO_CMAKE_PATH "${_lvrs_hint_abs}" _lvrs_hint_abs)

    if(EXISTS "${_lvrs_hint_abs}/platforms" AND EXISTS "${_lvrs_hint_abs}/LVRSConfig.cmake")
        set(_lvrs_install_root "${_lvrs_hint_abs}")
    elseif(_lvrs_hint_abs MATCHES "^(.*)/platforms/[^/]+/lib/cmake/LVRS$")
        set(_lvrs_install_root "${CMAKE_MATCH_1}")
    elseif(_lvrs_hint_abs MATCHES "^(.*)/platforms/[^/]+$")
        set(_lvrs_install_root "${CMAKE_MATCH_1}")
    endif()

    set(${out_var} "${_lvrs_install_root}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_resolve_platform_lvrs_dir install_root platform out_var)
    set(_lvrs_platform_dir "")
    if(NOT install_root STREQUAL "")
        set(_lvrs_candidate "${install_root}/platforms/${platform}/lib/cmake/LVRS")
        if(EXISTS "${_lvrs_candidate}/LVRSConfig.cmake")
            set(_lvrs_platform_dir "${_lvrs_candidate}")
        endif()
    endif()

    set(${out_var} "${_lvrs_platform_dir}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_resolve_bootstrap_lvrs_dir hint platform out_var)
    _lvrs_internal_resolve_install_root_from_lvrs_hint("${hint}" _lvrs_install_root)
    if(NOT _lvrs_install_root STREQUAL "")
        _lvrs_internal_resolve_platform_lvrs_dir(
            "${_lvrs_install_root}"
            "${platform}"
            _lvrs_platform_dir
        )
        if(NOT _lvrs_platform_dir STREQUAL "")
            set(${out_var} "${_lvrs_platform_dir}" PARENT_SCOPE)
            return()
        endif()
    endif()

    set(${out_var} "${hint}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_framework_bootstrap_install_root out_var)
    set(_lvrs_install_root "")
    if(DEFINED LVRS_BOOTSTRAP_INSTALL_ROOT AND NOT LVRS_BOOTSTRAP_INSTALL_ROOT STREQUAL "")
        set(_lvrs_install_root "${LVRS_BOOTSTRAP_INSTALL_ROOT}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_INSTALL_ROOT} AND NOT "$ENV{LVRS_BOOTSTRAP_INSTALL_ROOT}" STREQUAL "")
        set(_lvrs_install_root "$ENV{LVRS_BOOTSTRAP_INSTALL_ROOT}")
    elseif(DEFINED LVRS_BOOTSTRAP_INSTALL_PREFIX AND NOT LVRS_BOOTSTRAP_INSTALL_PREFIX STREQUAL "")
        # Backward-compatible alias: treat LVRS_BOOTSTRAP_INSTALL_PREFIX as install root.
        set(_lvrs_install_root "${LVRS_BOOTSTRAP_INSTALL_PREFIX}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_INSTALL_PREFIX} AND NOT "$ENV{LVRS_BOOTSTRAP_INSTALL_PREFIX}" STREQUAL "")
        set(_lvrs_install_root "$ENV{LVRS_BOOTSTRAP_INSTALL_PREFIX}")
    else()
        set(_lvrs_install_root "${CMAKE_BINARY_DIR}/lvrs-install")
    endif()

    set(${out_var} "${_lvrs_install_root}" PARENT_SCOPE)
endfunction()

function(_lvrs_internal_framework_bootstrap_install_prefix_for_platform platform install_root out_var)
    string(TOUPPER "${platform}" _lvrs_upper)
    set(_lvrs_override_var "LVRS_BOOTSTRAP_INSTALL_PREFIX_${_lvrs_upper}")
    set(_lvrs_install_prefix "")

    if(DEFINED ${_lvrs_override_var} AND NOT "${${_lvrs_override_var}}" STREQUAL "")
        set(_lvrs_install_prefix "${${_lvrs_override_var}}")
    elseif(DEFINED ENV{${_lvrs_override_var}} AND NOT "$ENV{${_lvrs_override_var}}" STREQUAL "")
        set(_lvrs_install_prefix "$ENV{${_lvrs_override_var}}")
    endif()

    if(_lvrs_install_prefix STREQUAL "")
        set(_lvrs_install_prefix "${install_root}/${platform}")
    endif()

    set(${out_var} "${_lvrs_install_prefix}" PARENT_SCOPE)
endfunction()

function(lvrs_create_framework_bootstrap_targets)
    _lvrs_internal_framework_bootstrap_platforms(_lvrs_runtime_platforms)

    set(_lvrs_bootstrap_root "")
    if(DEFINED LVRS_BOOTSTRAP_ROOT_DIR AND NOT LVRS_BOOTSTRAP_ROOT_DIR STREQUAL "")
        set(_lvrs_bootstrap_root "${LVRS_BOOTSTRAP_ROOT_DIR}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_ROOT_DIR} AND NOT "$ENV{LVRS_BOOTSTRAP_ROOT_DIR}" STREQUAL "")
        set(_lvrs_bootstrap_root "$ENV{LVRS_BOOTSTRAP_ROOT_DIR}")
    else()
        set(_lvrs_bootstrap_root "${CMAKE_BINARY_DIR}/lvrs-bootstrap")
    endif()

    set(_lvrs_bootstrap_source_dir "")
    if(DEFINED LVRS_BOOTSTRAP_SOURCE_DIR AND NOT LVRS_BOOTSTRAP_SOURCE_DIR STREQUAL "")
        set(_lvrs_bootstrap_source_dir "${LVRS_BOOTSTRAP_SOURCE_DIR}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_SOURCE_DIR} AND NOT "$ENV{LVRS_BOOTSTRAP_SOURCE_DIR}" STREQUAL "")
        set(_lvrs_bootstrap_source_dir "$ENV{LVRS_BOOTSTRAP_SOURCE_DIR}")
    else()
        set(_lvrs_bootstrap_source_dir "${CMAKE_SOURCE_DIR}")
    endif()

    set(_lvrs_bootstrap_script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/LVRSBootstrapFrameworkAction.cmake")
    if(NOT EXISTS "${_lvrs_bootstrap_script}")
        message(FATAL_ERROR "LVRS framework bootstrap helper script not found: ${_lvrs_bootstrap_script}")
    endif()

    _lvrs_internal_bootstrap_build_type(_lvrs_bootstrap_build_type)
    _lvrs_internal_framework_bootstrap_install_root(_lvrs_framework_install_root)

    set(_lvrs_bootstrap_find_no_pkg_registry "")
    if(DEFINED CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY)
        if(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY)
            set(_lvrs_bootstrap_find_no_pkg_registry "ON")
        else()
            set(_lvrs_bootstrap_find_no_pkg_registry "OFF")
        endif()
    endif()

    set(_lvrs_bootstrap_find_use_pkg_registry "")
    if(DEFINED CMAKE_FIND_USE_PACKAGE_REGISTRY)
        if(CMAKE_FIND_USE_PACKAGE_REGISTRY)
            set(_lvrs_bootstrap_find_use_pkg_registry "ON")
        else()
            set(_lvrs_bootstrap_find_use_pkg_registry "OFF")
        endif()
    endif()

    set(_lvrs_bootstrap_emsdk_root "")
    if(DEFINED LVRS_BOOTSTRAP_EMSDK_ROOT AND NOT LVRS_BOOTSTRAP_EMSDK_ROOT STREQUAL "")
        set(_lvrs_bootstrap_emsdk_root "${LVRS_BOOTSTRAP_EMSDK_ROOT}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_EMSDK_ROOT} AND NOT "$ENV{LVRS_BOOTSTRAP_EMSDK_ROOT}" STREQUAL "")
        set(_lvrs_bootstrap_emsdk_root "$ENV{LVRS_BOOTSTRAP_EMSDK_ROOT}")
    elseif(DEFINED LVRS_EMSDK_HINT AND NOT LVRS_EMSDK_HINT STREQUAL "")
        set(_lvrs_bootstrap_emsdk_root "${LVRS_EMSDK_HINT}")
    elseif(DEFINED ENV{EMSDK} AND NOT "$ENV{EMSDK}" STREQUAL "")
        set(_lvrs_bootstrap_emsdk_root "$ENV{EMSDK}")
    endif()

    set(_lvrs_bootstrap_emscripten_toolchain_file "")
    if(DEFINED LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE
       AND NOT LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE STREQUAL "")
        set(_lvrs_bootstrap_emscripten_toolchain_file "${LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE}
           AND NOT "$ENV{LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE}" STREQUAL "")
        set(_lvrs_bootstrap_emscripten_toolchain_file "$ENV{LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE}")
    endif()

    set(_lvrs_bootstrap_lvrs_build_examples "OFF")
    if(DEFINED LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES)
        set(_lvrs_bootstrap_lvrs_build_examples "${LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES} AND NOT "$ENV{LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES}" STREQUAL "")
        set(_lvrs_bootstrap_lvrs_build_examples "$ENV{LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES}")
    endif()

    set(_lvrs_bootstrap_lvrs_build_tests "OFF")
    if(DEFINED LVRS_BOOTSTRAP_LVRS_BUILD_TESTS)
        set(_lvrs_bootstrap_lvrs_build_tests "${LVRS_BOOTSTRAP_LVRS_BUILD_TESTS}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_LVRS_BUILD_TESTS} AND NOT "$ENV{LVRS_BOOTSTRAP_LVRS_BUILD_TESTS}" STREQUAL "")
        set(_lvrs_bootstrap_lvrs_build_tests "$ENV{LVRS_BOOTSTRAP_LVRS_BUILD_TESTS}")
    endif()

    set(_lvrs_bootstrap_lvrs_build_shared_libs "")
    if(DEFINED LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS)
        set(_lvrs_bootstrap_lvrs_build_shared_libs "${LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS}
           AND NOT "$ENV{LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS}" STREQUAL "")
        set(_lvrs_bootstrap_lvrs_build_shared_libs "$ENV{LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS}")
    elseif(DEFINED LVRS_BUILD_SHARED_LIBS)
        if(LVRS_BUILD_SHARED_LIBS)
            set(_lvrs_bootstrap_lvrs_build_shared_libs "ON")
        else()
            set(_lvrs_bootstrap_lvrs_build_shared_libs "OFF")
        endif()
    endif()

    set(_lvrs_bootstrap_lvrs_install_qml_module "")
    if(DEFINED LVRS_BOOTSTRAP_LVRS_INSTALL_QML_MODULE)
        set(_lvrs_bootstrap_lvrs_install_qml_module "${LVRS_BOOTSTRAP_LVRS_INSTALL_QML_MODULE}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_LVRS_INSTALL_QML_MODULE}
           AND NOT "$ENV{LVRS_BOOTSTRAP_LVRS_INSTALL_QML_MODULE}" STREQUAL "")
        set(_lvrs_bootstrap_lvrs_install_qml_module "$ENV{LVRS_BOOTSTRAP_LVRS_INSTALL_QML_MODULE}")
    elseif(DEFINED LVRS_INSTALL_QML_MODULE)
        if(LVRS_INSTALL_QML_MODULE)
            set(_lvrs_bootstrap_lvrs_install_qml_module "ON")
        else()
            set(_lvrs_bootstrap_lvrs_install_qml_module "OFF")
        endif()
    endif()

    set(_lvrs_bootstrap_lvrs_enforce_vulkan "")
    if(DEFINED LVRS_BOOTSTRAP_LVRS_ENFORCE_VULKAN)
        set(_lvrs_bootstrap_lvrs_enforce_vulkan "${LVRS_BOOTSTRAP_LVRS_ENFORCE_VULKAN}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_LVRS_ENFORCE_VULKAN}
           AND NOT "$ENV{LVRS_BOOTSTRAP_LVRS_ENFORCE_VULKAN}" STREQUAL "")
        set(_lvrs_bootstrap_lvrs_enforce_vulkan "$ENV{LVRS_BOOTSTRAP_LVRS_ENFORCE_VULKAN}")
    elseif(DEFINED LVRS_ENFORCE_VULKAN)
        if(LVRS_ENFORCE_VULKAN)
            set(_lvrs_bootstrap_lvrs_enforce_vulkan "ON")
        else()
            set(_lvrs_bootstrap_lvrs_enforce_vulkan "OFF")
        endif()
    endif()

    set(_lvrs_bootstrap_lvrs_enable_platform_build_optimizations "")
    if(DEFINED LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS)
        set(_lvrs_bootstrap_lvrs_enable_platform_build_optimizations
            "${LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS}
           AND NOT "$ENV{LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS}" STREQUAL "")
        set(_lvrs_bootstrap_lvrs_enable_platform_build_optimizations
            "$ENV{LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS}")
    elseif(DEFINED LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS)
        if(LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS)
            set(_lvrs_bootstrap_lvrs_enable_platform_build_optimizations "ON")
        else()
            set(_lvrs_bootstrap_lvrs_enable_platform_build_optimizations "OFF")
        endif()
    endif()

    set(_lvrs_bootstrap_lvrs_enable_ipo "")
    if(DEFINED LVRS_BOOTSTRAP_LVRS_ENABLE_IPO)
        set(_lvrs_bootstrap_lvrs_enable_ipo "${LVRS_BOOTSTRAP_LVRS_ENABLE_IPO}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_LVRS_ENABLE_IPO}
           AND NOT "$ENV{LVRS_BOOTSTRAP_LVRS_ENABLE_IPO}" STREQUAL "")
        set(_lvrs_bootstrap_lvrs_enable_ipo "$ENV{LVRS_BOOTSTRAP_LVRS_ENABLE_IPO}")
    elseif(DEFINED LVRS_ENABLE_IPO)
        if(LVRS_ENABLE_IPO)
            set(_lvrs_bootstrap_lvrs_enable_ipo "ON")
        else()
            set(_lvrs_bootstrap_lvrs_enable_ipo "OFF")
        endif()
    endif()

    set(_lvrs_bootstrap_android_sdk_root "")
    if(DEFINED LVRS_BOOTSTRAP_ANDROID_SDK_ROOT AND NOT LVRS_BOOTSTRAP_ANDROID_SDK_ROOT STREQUAL "")
        set(_lvrs_bootstrap_android_sdk_root "${LVRS_BOOTSTRAP_ANDROID_SDK_ROOT}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_ANDROID_SDK_ROOT} AND NOT "$ENV{LVRS_BOOTSTRAP_ANDROID_SDK_ROOT}" STREQUAL "")
        set(_lvrs_bootstrap_android_sdk_root "$ENV{LVRS_BOOTSTRAP_ANDROID_SDK_ROOT}")
    endif()

    set(_lvrs_bootstrap_android_ndk "")
    if(DEFINED LVRS_BOOTSTRAP_ANDROID_NDK AND NOT LVRS_BOOTSTRAP_ANDROID_NDK STREQUAL "")
        set(_lvrs_bootstrap_android_ndk "${LVRS_BOOTSTRAP_ANDROID_NDK}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_ANDROID_NDK} AND NOT "$ENV{LVRS_BOOTSTRAP_ANDROID_NDK}" STREQUAL "")
        set(_lvrs_bootstrap_android_ndk "$ENV{LVRS_BOOTSTRAP_ANDROID_NDK}")
    endif()

    set(_lvrs_ios_architectures "")
    if(DEFINED LVRS_BOOTSTRAP_IOS_ARCHITECTURES)
        set(_lvrs_ios_architectures "${LVRS_BOOTSTRAP_IOS_ARCHITECTURES}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_IOS_ARCHITECTURES} AND NOT "$ENV{LVRS_BOOTSTRAP_IOS_ARCHITECTURES}" STREQUAL "")
        set(_lvrs_ios_architectures "$ENV{LVRS_BOOTSTRAP_IOS_ARCHITECTURES}")
    endif()
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
        _lvrs_internal_filter_apple_x86_architectures("${_lvrs_ios_architectures}" _lvrs_ios_architectures_filtered)
        if(NOT _lvrs_ios_architectures STREQUAL "${_lvrs_ios_architectures_filtered}")
            message(STATUS "LVRS framework bootstrap targets: removed Apple x86 iOS architectures from LVRS_BOOTSTRAP_IOS_ARCHITECTURES.")
        endif()
        if(_lvrs_ios_architectures_filtered STREQUAL "")
            set(_lvrs_ios_architectures "arm64")
        else()
            set(_lvrs_ios_architectures "${_lvrs_ios_architectures_filtered}")
        endif()
    endif()

    set(_lvrs_ios_code_signing "")
    if(DEFINED LVRS_BOOTSTRAP_IOS_CODE_SIGNING)
        set(_lvrs_ios_code_signing "${LVRS_BOOTSTRAP_IOS_CODE_SIGNING}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_IOS_CODE_SIGNING} AND NOT "$ENV{LVRS_BOOTSTRAP_IOS_CODE_SIGNING}" STREQUAL "")
        set(_lvrs_ios_code_signing "$ENV{LVRS_BOOTSTRAP_IOS_CODE_SIGNING}")
    endif()

    set(_lvrs_ios_bundle_identifier "")
    if(DEFINED LVRS_BOOTSTRAP_IOS_BUNDLE_IDENTIFIER)
        set(_lvrs_ios_bundle_identifier "${LVRS_BOOTSTRAP_IOS_BUNDLE_IDENTIFIER}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_IOS_BUNDLE_IDENTIFIER} AND NOT "$ENV{LVRS_BOOTSTRAP_IOS_BUNDLE_IDENTIFIER}" STREQUAL "")
        set(_lvrs_ios_bundle_identifier "$ENV{LVRS_BOOTSTRAP_IOS_BUNDLE_IDENTIFIER}")
    endif()

    set(_lvrs_all_framework_bootstrap_targets "")
    foreach(_lvrs_platform IN LISTS _lvrs_runtime_platforms)
        set(_lvrs_bootstrap_target "bootstrap_lvrs_${_lvrs_platform}")
        if(TARGET "${_lvrs_bootstrap_target}")
            list(APPEND _lvrs_all_framework_bootstrap_targets "${_lvrs_bootstrap_target}")
            continue()
        endif()

        _lvrs_internal_bootstrap_system_name_for_platform("${_lvrs_platform}" _lvrs_system_name)
        _lvrs_internal_bootstrap_osx_sysroot_for_platform("${_lvrs_platform}" _lvrs_osx_sysroot)
        if(_lvrs_platform STREQUAL "ios")
            set(_lvrs_ios_sysroot_overridden FALSE)
            if(DEFINED LVRS_BOOTSTRAP_OSX_SYSROOT_IOS AND NOT LVRS_BOOTSTRAP_OSX_SYSROOT_IOS STREQUAL "")
                set(_lvrs_ios_sysroot_overridden TRUE)
            elseif(DEFINED ENV{LVRS_BOOTSTRAP_OSX_SYSROOT_IOS}
                   AND NOT "$ENV{LVRS_BOOTSTRAP_OSX_SYSROOT_IOS}" STREQUAL "")
                set(_lvrs_ios_sysroot_overridden TRUE)
            endif()

            if(NOT _lvrs_ios_sysroot_overridden AND _lvrs_osx_sysroot STREQUAL "iphonesimulator")
                set(_lvrs_osx_sysroot "iphoneos")
                message(STATUS "LVRS framework bootstrap targets: default iOS sysroot fallback iphonesimulator -> iphoneos.")
            endif()
        endif()
        _lvrs_internal_bootstrap_android_abi_for_platform("${_lvrs_platform}" _lvrs_android_abi)
        _lvrs_internal_detect_qt_prefix_for_platform("${_lvrs_platform}" _lvrs_qt_prefix)
        if(_lvrs_qt_prefix STREQUAL "")
            _lvrs_internal_report_missing_bootstrap_qt_kit("framework" "${_lvrs_platform}")
            continue()
        endif()
        _lvrs_internal_bootstrap_toolchain_for_platform("${_lvrs_platform}" "${_lvrs_qt_prefix}" _lvrs_toolchain_file)
        _lvrs_internal_bootstrap_generator_for_platform("${_lvrs_platform}" _lvrs_generator)

        set(_lvrs_combined_prefix_path "")
        if(NOT _lvrs_qt_prefix STREQUAL "")
            set(_lvrs_combined_prefix_path "${_lvrs_qt_prefix}")
        endif()
        if(NOT CMAKE_PREFIX_PATH STREQUAL "")
            if(_lvrs_combined_prefix_path STREQUAL "")
                set(_lvrs_combined_prefix_path "${CMAKE_PREFIX_PATH}")
            else()
                set(_lvrs_combined_prefix_path "${_lvrs_combined_prefix_path};${CMAKE_PREFIX_PATH}")
            endif()
        endif()
        _lvrs_internal_escape_list_for_cache("${_lvrs_combined_prefix_path}" _lvrs_combined_prefix_path_escaped)

        set(_lvrs_platform_build_dir "${_lvrs_bootstrap_root}/framework/${_lvrs_platform}")
        _lvrs_internal_framework_bootstrap_install_prefix_for_platform(
            "${_lvrs_platform}"
            "${_lvrs_framework_install_root}"
            _lvrs_platform_install_prefix
        )

        add_custom_target("${_lvrs_bootstrap_target}"
            COMMAND "${CMAKE_COMMAND}"
                "-DLVRS_BOOTSTRAP_SOURCE_DIR=${_lvrs_bootstrap_source_dir}"
                "-DLVRS_BOOTSTRAP_BINARY_DIR=${_lvrs_platform_build_dir}"
                "-DLVRS_BOOTSTRAP_PLATFORM=${_lvrs_platform}"
                "-DLVRS_BOOTSTRAP_SYSTEM_NAME=${_lvrs_system_name}"
                "-DLVRS_BOOTSTRAP_PREFIX_PATH=${_lvrs_combined_prefix_path_escaped}"
                "-DLVRS_BOOTSTRAP_TOOLCHAIN_FILE=${_lvrs_toolchain_file}"
                "-DLVRS_BOOTSTRAP_GENERATOR=${_lvrs_generator}"
                "-DLVRS_BOOTSTRAP_BUILD_TYPE=${_lvrs_bootstrap_build_type}"
                "-DLVRS_BOOTSTRAP_OSX_SYSROOT=${_lvrs_osx_sysroot}"
                "-DLVRS_BOOTSTRAP_ANDROID_ABI=${_lvrs_android_abi}"
                "-DLVRS_BOOTSTRAP_ANDROID_SDK_ROOT=${_lvrs_bootstrap_android_sdk_root}"
                "-DLVRS_BOOTSTRAP_ANDROID_NDK=${_lvrs_bootstrap_android_ndk}"
                "-DLVRS_BOOTSTRAP_EMSDK_ROOT=${_lvrs_bootstrap_emsdk_root}"
                "-DLVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE=${_lvrs_bootstrap_emscripten_toolchain_file}"
                "-DLVRS_BOOTSTRAP_INSTALL_PREFIX=${_lvrs_platform_install_prefix}"
                "-DLVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES=${_lvrs_bootstrap_lvrs_build_examples}"
                "-DLVRS_BOOTSTRAP_LVRS_BUILD_TESTS=${_lvrs_bootstrap_lvrs_build_tests}"
                "-DLVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS=${_lvrs_bootstrap_lvrs_build_shared_libs}"
                "-DLVRS_BOOTSTRAP_LVRS_INSTALL_QML_MODULE=${_lvrs_bootstrap_lvrs_install_qml_module}"
                "-DLVRS_BOOTSTRAP_LVRS_ENFORCE_VULKAN=${_lvrs_bootstrap_lvrs_enforce_vulkan}"
                "-DLVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS=${_lvrs_bootstrap_lvrs_enable_platform_build_optimizations}"
                "-DLVRS_BOOTSTRAP_LVRS_ENABLE_IPO=${_lvrs_bootstrap_lvrs_enable_ipo}"
                "-DLVRS_BOOTSTRAP_FIND_PACKAGE_NO_PACKAGE_REGISTRY=${_lvrs_bootstrap_find_no_pkg_registry}"
                "-DLVRS_BOOTSTRAP_FIND_USE_PACKAGE_REGISTRY=${_lvrs_bootstrap_find_use_pkg_registry}"
                "-DLVRS_BOOTSTRAP_IOS_ARCHITECTURES=${_lvrs_ios_architectures}"
                "-DLVRS_BOOTSTRAP_IOS_CODE_SIGNING=${_lvrs_ios_code_signing}"
                "-DLVRS_BOOTSTRAP_IOS_BUNDLE_IDENTIFIER=${_lvrs_ios_bundle_identifier}"
                -P "${_lvrs_bootstrap_script}"
            USES_TERMINAL
        )
        set_property(TARGET "${_lvrs_bootstrap_target}" PROPERTY FOLDER "LVRS/FrameworkBootstrapTargets")
        list(APPEND _lvrs_all_framework_bootstrap_targets "${_lvrs_bootstrap_target}")
    endforeach()

    if(NOT TARGET bootstrap_lvrs_all)
        add_custom_target(bootstrap_lvrs_all DEPENDS ${_lvrs_all_framework_bootstrap_targets})
        set_property(TARGET bootstrap_lvrs_all PROPERTY FOLDER "LVRS/FrameworkBootstrapTargets")
    endif()
endfunction()

function(_lvrs_internal_create_platform_bootstrap_targets target)
    _lvrs_internal_known_runtime_platforms(_lvrs_runtime_platforms)

    set(_lvrs_bootstrap_root "")
    if(DEFINED LVRS_BOOTSTRAP_ROOT_DIR AND NOT LVRS_BOOTSTRAP_ROOT_DIR STREQUAL "")
        set(_lvrs_bootstrap_root "${LVRS_BOOTSTRAP_ROOT_DIR}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_ROOT_DIR} AND NOT "$ENV{LVRS_BOOTSTRAP_ROOT_DIR}" STREQUAL "")
        set(_lvrs_bootstrap_root "$ENV{LVRS_BOOTSTRAP_ROOT_DIR}")
    else()
        set(_lvrs_bootstrap_root "${CMAKE_BINARY_DIR}/lvrs-bootstrap")
    endif()

    set(_lvrs_bootstrap_source_dir "")
    if(DEFINED LVRS_BOOTSTRAP_SOURCE_DIR AND NOT LVRS_BOOTSTRAP_SOURCE_DIR STREQUAL "")
        set(_lvrs_bootstrap_source_dir "${LVRS_BOOTSTRAP_SOURCE_DIR}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_SOURCE_DIR} AND NOT "$ENV{LVRS_BOOTSTRAP_SOURCE_DIR}" STREQUAL "")
        set(_lvrs_bootstrap_source_dir "$ENV{LVRS_BOOTSTRAP_SOURCE_DIR}")
    else()
        set(_lvrs_bootstrap_source_dir "${CMAKE_SOURCE_DIR}")
    endif()

    set(_lvrs_bootstrap_script "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/LVRSBootstrapAction.cmake")
    if(NOT EXISTS "${_lvrs_bootstrap_script}")
        message(FATAL_ERROR "LVRS bootstrap helper script not found: ${_lvrs_bootstrap_script}")
    endif()

    _lvrs_internal_bootstrap_build_type(_lvrs_bootstrap_build_type)
    _lvrs_internal_detect_qt_host_prefix(_lvrs_qt_host_prefix)

    set(_lvrs_bootstrap_lvrs_hint "")
    if(DEFINED LVRS_DIR AND NOT LVRS_DIR STREQUAL "")
        set(_lvrs_bootstrap_lvrs_hint "${LVRS_DIR}")
    elseif(DEFINED ENV{LVRS_DIR} AND NOT "$ENV{LVRS_DIR}" STREQUAL "")
        set(_lvrs_bootstrap_lvrs_hint "$ENV{LVRS_DIR}")
    elseif(DEFINED LVRS_INSTALL_ROOT AND NOT LVRS_INSTALL_ROOT STREQUAL "")
        set(_lvrs_bootstrap_lvrs_hint "${LVRS_INSTALL_ROOT}")
    elseif(DEFINED LVRS_ACTIVE_PREFIX AND NOT LVRS_ACTIVE_PREFIX STREQUAL "")
        set(_lvrs_bootstrap_lvrs_hint "${LVRS_ACTIVE_PREFIX}")
    endif()

    set(_lvrs_bootstrap_find_no_pkg_registry "")
    if(DEFINED CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY)
        if(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY)
            set(_lvrs_bootstrap_find_no_pkg_registry "ON")
        else()
            set(_lvrs_bootstrap_find_no_pkg_registry "OFF")
        endif()
    endif()

    set(_lvrs_bootstrap_find_use_pkg_registry "")
    if(DEFINED CMAKE_FIND_USE_PACKAGE_REGISTRY)
        if(CMAKE_FIND_USE_PACKAGE_REGISTRY)
            set(_lvrs_bootstrap_find_use_pkg_registry "ON")
        else()
            set(_lvrs_bootstrap_find_use_pkg_registry "OFF")
        endif()
    endif()

    set(_lvrs_bootstrap_emsdk_root "")
    if(DEFINED LVRS_BOOTSTRAP_EMSDK_ROOT AND NOT LVRS_BOOTSTRAP_EMSDK_ROOT STREQUAL "")
        set(_lvrs_bootstrap_emsdk_root "${LVRS_BOOTSTRAP_EMSDK_ROOT}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_EMSDK_ROOT} AND NOT "$ENV{LVRS_BOOTSTRAP_EMSDK_ROOT}" STREQUAL "")
        set(_lvrs_bootstrap_emsdk_root "$ENV{LVRS_BOOTSTRAP_EMSDK_ROOT}")
    elseif(DEFINED LVRS_EMSDK_HINT AND NOT LVRS_EMSDK_HINT STREQUAL "")
        set(_lvrs_bootstrap_emsdk_root "${LVRS_EMSDK_HINT}")
    elseif(DEFINED ENV{EMSDK} AND NOT "$ENV{EMSDK}" STREQUAL "")
        set(_lvrs_bootstrap_emsdk_root "$ENV{EMSDK}")
    endif()

    set(_lvrs_bootstrap_emscripten_toolchain_file "")
    if(DEFINED LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE
       AND NOT LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE STREQUAL "")
        set(_lvrs_bootstrap_emscripten_toolchain_file "${LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE}
           AND NOT "$ENV{LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE}" STREQUAL "")
        set(_lvrs_bootstrap_emscripten_toolchain_file "$ENV{LVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE}")
    endif()

    set(_lvrs_all_bootstrap_targets "")
    foreach(_lvrs_platform IN LISTS _lvrs_runtime_platforms)
        set(_lvrs_bootstrap_target "bootstrap_${target}_${_lvrs_platform}")
        if(TARGET "${_lvrs_bootstrap_target}")
            list(APPEND _lvrs_all_bootstrap_targets "${_lvrs_bootstrap_target}")
            continue()
        endif()

        _lvrs_internal_bootstrap_system_name_for_platform("${_lvrs_platform}" _lvrs_system_name)
        _lvrs_internal_bootstrap_osx_sysroot_for_platform("${_lvrs_platform}" _lvrs_osx_sysroot)
        _lvrs_internal_bootstrap_android_abi_for_platform("${_lvrs_platform}" _lvrs_android_abi)
        _lvrs_internal_detect_qt_prefix_for_platform("${_lvrs_platform}" _lvrs_qt_prefix)
        if(_lvrs_qt_prefix STREQUAL "")
            _lvrs_internal_report_missing_bootstrap_qt_kit("app" "${_lvrs_platform}")
            continue()
        endif()
        _lvrs_internal_bootstrap_toolchain_for_platform("${_lvrs_platform}" "${_lvrs_qt_prefix}" _lvrs_toolchain_file)
        _lvrs_internal_bootstrap_generator_for_platform("${_lvrs_platform}" _lvrs_generator)

        set(_lvrs_combined_prefix_path "")
        if(NOT _lvrs_qt_prefix STREQUAL "")
            set(_lvrs_combined_prefix_path "${_lvrs_qt_prefix}")
        endif()
        if(NOT CMAKE_PREFIX_PATH STREQUAL "")
            if(_lvrs_combined_prefix_path STREQUAL "")
                set(_lvrs_combined_prefix_path "${CMAKE_PREFIX_PATH}")
            else()
                set(_lvrs_combined_prefix_path "${_lvrs_combined_prefix_path};${CMAKE_PREFIX_PATH}")
            endif()
        endif()

        _lvrs_internal_escape_list_for_cache("${_lvrs_combined_prefix_path}" _lvrs_combined_prefix_path_escaped)

        set(_lvrs_platform_build_dir "${_lvrs_bootstrap_root}/${target}/${_lvrs_platform}")
        _lvrs_internal_resolve_bootstrap_lvrs_dir(
            "${_lvrs_bootstrap_lvrs_hint}"
            "${_lvrs_platform}"
            _lvrs_platform_lvrs_dir
        )

        set(_lvrs_generate_ios_xcode_project OFF)
        if(_lvrs_platform STREQUAL "ios")
            if(DEFINED LVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT)
                set(_lvrs_generate_ios_xcode_project "${LVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT}")
            elseif(DEFINED ENV{LVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT})
                set(_lvrs_generate_ios_xcode_project "$ENV{LVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT}")
            else()
                set(_lvrs_generate_ios_xcode_project ON)
            endif()
        endif()

        set(_lvrs_generate_android_studio_project OFF)
        set(_lvrs_android_studio_project_dir "")
        if(_lvrs_platform STREQUAL "android")
            if(DEFINED LVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT)
                set(_lvrs_generate_android_studio_project "${LVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT}")
            elseif(DEFINED ENV{LVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT})
                set(_lvrs_generate_android_studio_project "$ENV{LVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT}")
            else()
                set(_lvrs_generate_android_studio_project ON)
            endif()

            if(DEFINED LVRS_ANDROID_STUDIO_PROJECT_DIR AND NOT LVRS_ANDROID_STUDIO_PROJECT_DIR STREQUAL "")
                set(_lvrs_android_studio_project_dir "${LVRS_ANDROID_STUDIO_PROJECT_DIR}")
            elseif(DEFINED ENV{LVRS_ANDROID_STUDIO_PROJECT_DIR} AND NOT "$ENV{LVRS_ANDROID_STUDIO_PROJECT_DIR}" STREQUAL "")
                set(_lvrs_android_studio_project_dir "$ENV{LVRS_ANDROID_STUDIO_PROJECT_DIR}")
            else()
                set(_lvrs_android_studio_project_dir "${_lvrs_platform_build_dir}/android-studio")
            endif()
        endif()

        set(_lvrs_androiddeployqt_path "")
        if(DEFINED LVRS_BOOTSTRAP_ANDROIDDEPLOYQT AND NOT LVRS_BOOTSTRAP_ANDROIDDEPLOYQT STREQUAL "")
            set(_lvrs_androiddeployqt_path "${LVRS_BOOTSTRAP_ANDROIDDEPLOYQT}")
        elseif(DEFINED ENV{LVRS_BOOTSTRAP_ANDROIDDEPLOYQT} AND NOT "$ENV{LVRS_BOOTSTRAP_ANDROIDDEPLOYQT}" STREQUAL "")
            set(_lvrs_androiddeployqt_path "$ENV{LVRS_BOOTSTRAP_ANDROIDDEPLOYQT}")
        endif()

        set(_lvrs_ios_simulator_name "")
        if(DEFINED LVRS_IOS_SIMULATOR_NAME AND NOT LVRS_IOS_SIMULATOR_NAME STREQUAL "")
            set(_lvrs_ios_simulator_name "${LVRS_IOS_SIMULATOR_NAME}")
        elseif(DEFINED ENV{LVRS_IOS_SIMULATOR_NAME} AND NOT "$ENV{LVRS_IOS_SIMULATOR_NAME}" STREQUAL "")
            set(_lvrs_ios_simulator_name "$ENV{LVRS_IOS_SIMULATOR_NAME}")
        else()
            set(_lvrs_ios_simulator_name "iPhone 17 Pro")
        endif()

        set(_lvrs_android_serial "")
        if(DEFINED LVRS_ANDROID_EMULATOR_SERIAL AND NOT LVRS_ANDROID_EMULATOR_SERIAL STREQUAL "")
            set(_lvrs_android_serial "${LVRS_ANDROID_EMULATOR_SERIAL}")
        elseif(DEFINED ENV{LVRS_ANDROID_EMULATOR_SERIAL} AND NOT "$ENV{LVRS_ANDROID_EMULATOR_SERIAL}" STREQUAL "")
            set(_lvrs_android_serial "$ENV{LVRS_ANDROID_EMULATOR_SERIAL}")
        elseif(DEFINED ENV{ANDROID_SERIAL} AND NOT "$ENV{ANDROID_SERIAL}" STREQUAL "")
            set(_lvrs_android_serial "$ENV{ANDROID_SERIAL}")
        else()
            set(_lvrs_android_serial "emulator-5554")
        endif()

        set(_lvrs_bootstrap_lvrs_build_examples "")
        if(DEFINED LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES)
            set(_lvrs_bootstrap_lvrs_build_examples "${LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES}")
        elseif(DEFINED ENV{LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES} AND NOT "$ENV{LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES}" STREQUAL "")
            set(_lvrs_bootstrap_lvrs_build_examples "$ENV{LVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES}")
        endif()

        set(_lvrs_bootstrap_lvrs_build_tests "")
        if(DEFINED LVRS_BOOTSTRAP_LVRS_BUILD_TESTS)
            set(_lvrs_bootstrap_lvrs_build_tests "${LVRS_BOOTSTRAP_LVRS_BUILD_TESTS}")
        elseif(DEFINED ENV{LVRS_BOOTSTRAP_LVRS_BUILD_TESTS} AND NOT "$ENV{LVRS_BOOTSTRAP_LVRS_BUILD_TESTS}" STREQUAL "")
            set(_lvrs_bootstrap_lvrs_build_tests "$ENV{LVRS_BOOTSTRAP_LVRS_BUILD_TESTS}")
        endif()

        set(_lvrs_bootstrap_lvrs_build_shared_libs "")
        if(DEFINED LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS)
            set(_lvrs_bootstrap_lvrs_build_shared_libs "${LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS}")
        elseif(DEFINED ENV{LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS}
               AND NOT "$ENV{LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS}" STREQUAL "")
            set(_lvrs_bootstrap_lvrs_build_shared_libs "$ENV{LVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS}")
        endif()

        set(_lvrs_bootstrap_lvrs_enable_platform_build_optimizations "")
        if(DEFINED LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS)
            set(_lvrs_bootstrap_lvrs_enable_platform_build_optimizations
                "${LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS}")
        elseif(DEFINED ENV{LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS}
               AND NOT "$ENV{LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS}" STREQUAL "")
            set(_lvrs_bootstrap_lvrs_enable_platform_build_optimizations
                "$ENV{LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS}")
        elseif(DEFINED LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS)
            if(LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS)
                set(_lvrs_bootstrap_lvrs_enable_platform_build_optimizations "ON")
            else()
                set(_lvrs_bootstrap_lvrs_enable_platform_build_optimizations "OFF")
            endif()
        endif()

        set(_lvrs_bootstrap_lvrs_enable_ipo "")
        if(DEFINED LVRS_BOOTSTRAP_LVRS_ENABLE_IPO)
            set(_lvrs_bootstrap_lvrs_enable_ipo "${LVRS_BOOTSTRAP_LVRS_ENABLE_IPO}")
        elseif(DEFINED ENV{LVRS_BOOTSTRAP_LVRS_ENABLE_IPO}
               AND NOT "$ENV{LVRS_BOOTSTRAP_LVRS_ENABLE_IPO}" STREQUAL "")
            set(_lvrs_bootstrap_lvrs_enable_ipo "$ENV{LVRS_BOOTSTRAP_LVRS_ENABLE_IPO}")
        elseif(DEFINED LVRS_ENABLE_IPO)
            if(LVRS_ENABLE_IPO)
                set(_lvrs_bootstrap_lvrs_enable_ipo "ON")
            else()
                set(_lvrs_bootstrap_lvrs_enable_ipo "OFF")
            endif()
        endif()

        set(_lvrs_ios_architectures "")
        if(DEFINED LVRS_BOOTSTRAP_IOS_ARCHITECTURES)
            set(_lvrs_ios_architectures "${LVRS_BOOTSTRAP_IOS_ARCHITECTURES}")
        elseif(DEFINED ENV{LVRS_BOOTSTRAP_IOS_ARCHITECTURES} AND NOT "$ENV{LVRS_BOOTSTRAP_IOS_ARCHITECTURES}" STREQUAL "")
            set(_lvrs_ios_architectures "$ENV{LVRS_BOOTSTRAP_IOS_ARCHITECTURES}")
        endif()
        if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
            _lvrs_internal_filter_apple_x86_architectures("${_lvrs_ios_architectures}" _lvrs_ios_architectures_filtered)
            if(NOT _lvrs_ios_architectures STREQUAL "${_lvrs_ios_architectures_filtered}")
                message(STATUS "LVRS app bootstrap targets: removed Apple x86 iOS architectures from LVRS_BOOTSTRAP_IOS_ARCHITECTURES.")
            endif()
            if(_lvrs_ios_architectures_filtered STREQUAL "")
                set(_lvrs_ios_architectures "arm64")
            else()
                set(_lvrs_ios_architectures "${_lvrs_ios_architectures_filtered}")
            endif()
        endif()

        add_custom_target("${_lvrs_bootstrap_target}"
            COMMAND "${CMAKE_COMMAND}"
                "-DLVRS_BOOTSTRAP_SOURCE_DIR=${_lvrs_bootstrap_source_dir}"
                "-DLVRS_BOOTSTRAP_BINARY_DIR=${_lvrs_platform_build_dir}"
                "-DLVRS_BOOTSTRAP_HOST_BUILD_DIR=${CMAKE_BINARY_DIR}"
                "-DLVRS_BOOTSTRAP_APP_TARGET=${target}"
                "-DLVRS_BOOTSTRAP_PLATFORM=${_lvrs_platform}"
                "-DLVRS_BOOTSTRAP_SYSTEM_NAME=${_lvrs_system_name}"
                "-DLVRS_BOOTSTRAP_PREFIX_PATH=${_lvrs_combined_prefix_path_escaped}"
                "-DLVRS_BOOTSTRAP_TOOLCHAIN_FILE=${_lvrs_toolchain_file}"
                "-DLVRS_BOOTSTRAP_GENERATOR=${_lvrs_generator}"
                "-DLVRS_BOOTSTRAP_BUILD_TYPE=${_lvrs_bootstrap_build_type}"
                "-DLVRS_BOOTSTRAP_QT_HOST_PREFIX=${_lvrs_qt_host_prefix}"
                "-DLVRS_BOOTSTRAP_OSX_SYSROOT=${_lvrs_osx_sysroot}"
                "-DLVRS_BOOTSTRAP_ANDROID_ABI=${_lvrs_android_abi}"
                "-DLVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT=${_lvrs_generate_ios_xcode_project}"
                "-DLVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT=${_lvrs_generate_android_studio_project}"
                "-DLVRS_BOOTSTRAP_ANDROID_STUDIO_PROJECT_DIR=${_lvrs_android_studio_project_dir}"
                "-DLVRS_BOOTSTRAP_ANDROIDDEPLOYQT=${_lvrs_androiddeployqt_path}"
                "-DLVRS_BOOTSTRAP_EMSDK_ROOT=${_lvrs_bootstrap_emsdk_root}"
                "-DLVRS_BOOTSTRAP_EMSCRIPTEN_TOOLCHAIN_FILE=${_lvrs_bootstrap_emscripten_toolchain_file}"
                "-DLVRS_BOOTSTRAP_LVRS_DIR=${_lvrs_platform_lvrs_dir}"
                "-DLVRS_BOOTSTRAP_FIND_PACKAGE_NO_PACKAGE_REGISTRY=${_lvrs_bootstrap_find_no_pkg_registry}"
                "-DLVRS_BOOTSTRAP_FIND_USE_PACKAGE_REGISTRY=${_lvrs_bootstrap_find_use_pkg_registry}"
                "-DLVRS_BOOTSTRAP_IOS_SIMULATOR_NAME=${_lvrs_ios_simulator_name}"
                "-DLVRS_BOOTSTRAP_ANDROID_SERIAL=${_lvrs_android_serial}"
                "-DLVRS_BOOTSTRAP_LVRS_BUILD_EXAMPLES=${_lvrs_bootstrap_lvrs_build_examples}"
                "-DLVRS_BOOTSTRAP_LVRS_BUILD_TESTS=${_lvrs_bootstrap_lvrs_build_tests}"
                "-DLVRS_BOOTSTRAP_LVRS_BUILD_SHARED_LIBS=${_lvrs_bootstrap_lvrs_build_shared_libs}"
                "-DLVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS=${_lvrs_bootstrap_lvrs_enable_platform_build_optimizations}"
                "-DLVRS_BOOTSTRAP_LVRS_ENABLE_IPO=${_lvrs_bootstrap_lvrs_enable_ipo}"
                "-DLVRS_BOOTSTRAP_IOS_ARCHITECTURES=${_lvrs_ios_architectures}"
                -P "${_lvrs_bootstrap_script}"
            USES_TERMINAL
        )
        set_property(TARGET "${_lvrs_bootstrap_target}" PROPERTY FOLDER "LVRS/BootstrapTargets")
        list(APPEND _lvrs_all_bootstrap_targets "${_lvrs_bootstrap_target}")
    endforeach()

    set(_lvrs_bootstrap_all_target "bootstrap_${target}_all")
    if(NOT TARGET "${_lvrs_bootstrap_all_target}")
        add_custom_target("${_lvrs_bootstrap_all_target}" DEPENDS ${_lvrs_all_bootstrap_targets})
        set_property(TARGET "${_lvrs_bootstrap_all_target}" PROPERTY FOLDER "LVRS/BootstrapTargets")
    endif()
endfunction()

function(_lvrs_internal_create_platform_runtime_targets target)
    _lvrs_internal_known_runtime_platforms(_lvrs_runtime_platforms)
    _lvrs_internal_detect_host_runtime_platform(_lvrs_host_platform)
    _lvrs_internal_detect_target_runtime_platform(_lvrs_target_platform)

    set_property(TARGET "${target}" PROPERTY LVRS_RUNTIME_TARGETS "${_lvrs_runtime_platforms}")
    set_property(TARGET "${target}" PROPERTY LVRS_HOST_RUNTIME_TARGET "${_lvrs_host_platform}")
    set_property(TARGET "${target}" PROPERTY LVRS_BUILD_TARGET_RUNTIME_PLATFORM "${_lvrs_target_platform}")

    foreach(_lvrs_platform IN LISTS _lvrs_runtime_platforms)
        set(_lvrs_run_target "run_${target}_${_lvrs_platform}")
        if(TARGET "${_lvrs_run_target}")
            continue()
        endif()

        _lvrs_internal_platform_to_cmake_system_name("${_lvrs_platform}" _lvrs_platform_system_name)
        _lvrs_internal_platform_supports_direct_run("${_lvrs_platform}" _lvrs_direct_run_supported)

        if(_lvrs_platform STREQUAL _lvrs_target_platform
           AND _lvrs_platform STREQUAL _lvrs_host_platform
           AND _lvrs_direct_run_supported)
            add_custom_target("${_lvrs_run_target}"
                COMMAND "$<TARGET_FILE:${target}>"
                DEPENDS "${target}"
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                USES_TERMINAL
            )
        else()
            if(_lvrs_platform STREQUAL _lvrs_host_platform)
                set(_lvrs_runtime_target_message
                    "LVRS generated '${_lvrs_run_target}'. '${_lvrs_platform}' target artifacts are ready; deployment is handled by platform tools.")
            else()
                set(_lvrs_runtime_target_message
                    "LVRS generated '${_lvrs_run_target}'. Reconfigure with -DCMAKE_SYSTEM_NAME=${_lvrs_platform_system_name} to produce runnable '${_lvrs_platform}' artifacts.")
            endif()

            add_custom_target("${_lvrs_run_target}"
                COMMAND "${CMAKE_COMMAND}" -E echo "${_lvrs_runtime_target_message}"
                USES_TERMINAL
            )
        endif()

        set_property(TARGET "${_lvrs_run_target}" PROPERTY FOLDER "LVRS/RuntimeTargets")
    endforeach()
endfunction()

function(_lvrs_internal_bootstrap_root_dir out_var)
    set(_lvrs_bootstrap_root "")
    if(DEFINED LVRS_BOOTSTRAP_ROOT_DIR AND NOT LVRS_BOOTSTRAP_ROOT_DIR STREQUAL "")
        set(_lvrs_bootstrap_root "${LVRS_BOOTSTRAP_ROOT_DIR}")
    elseif(DEFINED ENV{LVRS_BOOTSTRAP_ROOT_DIR} AND NOT "$ENV{LVRS_BOOTSTRAP_ROOT_DIR}" STREQUAL "")
        set(_lvrs_bootstrap_root "$ENV{LVRS_BOOTSTRAP_ROOT_DIR}")
    else()
        set(_lvrs_bootstrap_root "${CMAKE_BINARY_DIR}/lvrs-bootstrap")
    endif()
    set(${out_var} "${_lvrs_bootstrap_root}" PARENT_SCOPE)
endfunction()

function(lvrs_add_bootstrap_targets)
    set(_lvrs_one_value_args TARGET)
    cmake_parse_arguments(LVRS_BOOT "" "${_lvrs_one_value_args}" "" ${ARGN})

    if(NOT LVRS_BOOT_TARGET)
        message(FATAL_ERROR "lvrs_add_bootstrap_targets() requires TARGET")
    endif()
    if(NOT TARGET "${LVRS_BOOT_TARGET}")
        message(FATAL_ERROR "lvrs_add_bootstrap_targets() target not found: ${LVRS_BOOT_TARGET}")
    endif()

    _lvrs_internal_create_platform_bootstrap_targets("${LVRS_BOOT_TARGET}")
    _lvrs_internal_bootstrap_root_dir(_lvrs_bootstrap_root)

    set(_lvrs_ios_bootstrap_target "bootstrap_${LVRS_BOOT_TARGET}_ios")
    if(TARGET "${_lvrs_ios_bootstrap_target}")
        set(_lvrs_ios_launch_target "launch_${LVRS_BOOT_TARGET}_ios")
        if(NOT TARGET "${_lvrs_ios_launch_target}")
            add_custom_target("${_lvrs_ios_launch_target}" DEPENDS "${_lvrs_ios_bootstrap_target}")
            set_property(TARGET "${_lvrs_ios_launch_target}" PROPERTY FOLDER "LVRS/BootstrapTargets")
        endif()

        set(_lvrs_ios_export_target "export_${LVRS_BOOT_TARGET}_xcodeproj")
        if(NOT TARGET "${_lvrs_ios_export_target}")
            add_custom_target("${_lvrs_ios_export_target}"
                COMMAND "${CMAKE_COMMAND}"
                    "-DLVRS_EXPORT_KIND=xcodeproj"
                    "-DLVRS_EXPORT_TARGET=${LVRS_BOOT_TARGET}"
                    "-DLVRS_EXPORT_SOURCE_DIR=${_lvrs_bootstrap_root}/${LVRS_BOOT_TARGET}/ios"
                    "-DLVRS_EXPORT_OUTPUT_DIR=${CMAKE_BINARY_DIR}/lvrs-export/${LVRS_BOOT_TARGET}/xcodeproj"
                    -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/LVRSBootstrapExportAction.cmake"
                DEPENDS "${_lvrs_ios_bootstrap_target}"
                USES_TERMINAL
            )
            set_property(TARGET "${_lvrs_ios_export_target}" PROPERTY FOLDER "LVRS/BootstrapTargets")
        endif()
    endif()

    set(_lvrs_android_bootstrap_target "bootstrap_${LVRS_BOOT_TARGET}_android")
    if(TARGET "${_lvrs_android_bootstrap_target}")
        set(_lvrs_android_launch_target "launch_${LVRS_BOOT_TARGET}_android")
        if(NOT TARGET "${_lvrs_android_launch_target}")
            add_custom_target("${_lvrs_android_launch_target}" DEPENDS "${_lvrs_android_bootstrap_target}")
            set_property(TARGET "${_lvrs_android_launch_target}" PROPERTY FOLDER "LVRS/BootstrapTargets")
        endif()

        set(_lvrs_android_export_target "export_${LVRS_BOOT_TARGET}_android_studio")
        if(NOT TARGET "${_lvrs_android_export_target}")
            add_custom_target("${_lvrs_android_export_target}"
                COMMAND "${CMAKE_COMMAND}"
                    "-DLVRS_EXPORT_KIND=android_studio"
                    "-DLVRS_EXPORT_TARGET=${LVRS_BOOT_TARGET}"
                    "-DLVRS_EXPORT_SOURCE_DIR=${_lvrs_bootstrap_root}/${LVRS_BOOT_TARGET}/android/android-studio"
                    "-DLVRS_EXPORT_OUTPUT_DIR=${CMAKE_BINARY_DIR}/lvrs-export/${LVRS_BOOT_TARGET}/android_studio"
                    -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/LVRSBootstrapExportAction.cmake"
                DEPENDS "${_lvrs_android_bootstrap_target}"
                USES_TERMINAL
            )
            set_property(TARGET "${_lvrs_android_export_target}" PROPERTY FOLDER "LVRS/BootstrapTargets")
        endif()
    endif()

    set(_lvrs_wasm_bootstrap_target "bootstrap_${LVRS_BOOT_TARGET}_wasm")
    if(TARGET "${_lvrs_wasm_bootstrap_target}")
        set(_lvrs_wasm_launch_host "127.0.0.1")
        if(DEFINED LVRS_BOOTSTRAP_WASM_HOST AND NOT LVRS_BOOTSTRAP_WASM_HOST STREQUAL "")
            set(_lvrs_wasm_launch_host "${LVRS_BOOTSTRAP_WASM_HOST}")
        elseif(DEFINED ENV{LVRS_BOOTSTRAP_WASM_HOST} AND NOT "$ENV{LVRS_BOOTSTRAP_WASM_HOST}" STREQUAL "")
            set(_lvrs_wasm_launch_host "$ENV{LVRS_BOOTSTRAP_WASM_HOST}")
        endif()

        set(_lvrs_wasm_launch_port "8000")
        if(DEFINED LVRS_BOOTSTRAP_WASM_PORT AND NOT LVRS_BOOTSTRAP_WASM_PORT STREQUAL "")
            set(_lvrs_wasm_launch_port "${LVRS_BOOTSTRAP_WASM_PORT}")
        elseif(DEFINED ENV{LVRS_BOOTSTRAP_WASM_PORT} AND NOT "$ENV{LVRS_BOOTSTRAP_WASM_PORT}" STREQUAL "")
            set(_lvrs_wasm_launch_port "$ENV{LVRS_BOOTSTRAP_WASM_PORT}")
        endif()

        set(_lvrs_wasm_open_browser "ON")
        if(DEFINED LVRS_BOOTSTRAP_WASM_OPEN_BROWSER AND NOT LVRS_BOOTSTRAP_WASM_OPEN_BROWSER STREQUAL "")
            set(_lvrs_wasm_open_browser "${LVRS_BOOTSTRAP_WASM_OPEN_BROWSER}")
        elseif(DEFINED ENV{LVRS_BOOTSTRAP_WASM_OPEN_BROWSER} AND NOT "$ENV{LVRS_BOOTSTRAP_WASM_OPEN_BROWSER}" STREQUAL "")
            set(_lvrs_wasm_open_browser "$ENV{LVRS_BOOTSTRAP_WASM_OPEN_BROWSER}")
        endif()

        set(_lvrs_wasm_launch_target "launch_${LVRS_BOOT_TARGET}_wasm")
        if(NOT TARGET "${_lvrs_wasm_launch_target}")
            add_custom_target("${_lvrs_wasm_launch_target}"
                COMMAND "${CMAKE_COMMAND}"
                    "-DLVRS_WASM_LAUNCH_SOURCE_DIR=${_lvrs_bootstrap_root}/${LVRS_BOOT_TARGET}/wasm"
                    "-DLVRS_WASM_LAUNCH_TARGET=${LVRS_BOOT_TARGET}"
                    "-DLVRS_WASM_LAUNCH_HOST=${_lvrs_wasm_launch_host}"
                    "-DLVRS_WASM_LAUNCH_PORT=${_lvrs_wasm_launch_port}"
                    "-DLVRS_WASM_LAUNCH_OPEN_BROWSER=${_lvrs_wasm_open_browser}"
                    -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/LVRSBootstrapWasmLaunchAction.cmake"
                DEPENDS "${_lvrs_wasm_bootstrap_target}"
                USES_TERMINAL
            )
            set_property(TARGET "${_lvrs_wasm_launch_target}" PROPERTY FOLDER "LVRS/BootstrapTargets")
        endif()

        set(_lvrs_wasm_export_target "export_${LVRS_BOOT_TARGET}_wasm_site")
        if(NOT TARGET "${_lvrs_wasm_export_target}")
            add_custom_target("${_lvrs_wasm_export_target}"
                COMMAND "${CMAKE_COMMAND}"
                    "-DLVRS_EXPORT_KIND=wasm_site"
                    "-DLVRS_EXPORT_TARGET=${LVRS_BOOT_TARGET}"
                    "-DLVRS_EXPORT_SOURCE_DIR=${_lvrs_bootstrap_root}/${LVRS_BOOT_TARGET}/wasm"
                    "-DLVRS_EXPORT_OUTPUT_DIR=${CMAKE_BINARY_DIR}/lvrs-export/${LVRS_BOOT_TARGET}/wasm_site"
                    -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/LVRSBootstrapExportAction.cmake"
                DEPENDS "${_lvrs_wasm_bootstrap_target}"
                USES_TERMINAL
            )
            set_property(TARGET "${_lvrs_wasm_export_target}" PROPERTY FOLDER "LVRS/BootstrapTargets")
        endif()
    endif()
endfunction()

function(lvrs_configure_project_defaults)
    set(_lvrs_options IOS_EXCLUDE_QMLTOOLING)
    set(_lvrs_one_value_args
        TARGET
        APPLE_BUNDLE_ID
        APPLE_INFO_PLIST
        APPLE_ENTITLEMENTS
        ANDROID_PACKAGE_ID
        ANDROID_PACKAGE_SOURCE_DIR
        IOS_EXCLUDED_PLUGIN_TYPES
    )
    cmake_parse_arguments(LVRS_DEFAULTS
        "${_lvrs_options}"
        "${_lvrs_one_value_args}"
        ""
        ${ARGN}
    )

    if(NOT LVRS_DEFAULTS_TARGET)
        message(FATAL_ERROR "lvrs_configure_project_defaults() requires TARGET")
    endif()
    if(NOT TARGET "${LVRS_DEFAULTS_TARGET}")
        message(FATAL_ERROR "lvrs_configure_project_defaults() target not found: ${LVRS_DEFAULTS_TARGET}")
    endif()

    if(APPLE)
        if(NOT LVRS_DEFAULTS_APPLE_BUNDLE_ID STREQUAL "")
            set_target_properties("${LVRS_DEFAULTS_TARGET}" PROPERTIES
                MACOSX_BUNDLE_GUI_IDENTIFIER "${LVRS_DEFAULTS_APPLE_BUNDLE_ID}"
                XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${LVRS_DEFAULTS_APPLE_BUNDLE_ID}"
            )
        endif()
        if(NOT LVRS_DEFAULTS_APPLE_INFO_PLIST STREQUAL "")
            set_target_properties("${LVRS_DEFAULTS_TARGET}" PROPERTIES
                MACOSX_BUNDLE_INFO_PLIST "${LVRS_DEFAULTS_APPLE_INFO_PLIST}"
                XCODE_ATTRIBUTE_INFOPLIST_FILE "${LVRS_DEFAULTS_APPLE_INFO_PLIST}"
            )
        endif()
        if(NOT LVRS_DEFAULTS_APPLE_ENTITLEMENTS STREQUAL "")
            set_property(TARGET "${LVRS_DEFAULTS_TARGET}" PROPERTY
                XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${LVRS_DEFAULTS_APPLE_ENTITLEMENTS}")
        endif()
    endif()

    if(ANDROID)
        if(NOT LVRS_DEFAULTS_ANDROID_PACKAGE_SOURCE_DIR STREQUAL "")
            set_target_properties("${LVRS_DEFAULTS_TARGET}" PROPERTIES
                QT_ANDROID_PACKAGE_SOURCE_DIR "${LVRS_DEFAULTS_ANDROID_PACKAGE_SOURCE_DIR}"
            )
        endif()
        if(NOT LVRS_DEFAULTS_ANDROID_PACKAGE_ID STREQUAL "")
            set_target_properties("${LVRS_DEFAULTS_TARGET}" PROPERTIES
                QT_ANDROID_PACKAGE_NAME "${LVRS_DEFAULTS_ANDROID_PACKAGE_ID}"
            )
        endif()
    endif()

    set(_lvrs_ios_excluded_plugin_types "${LVRS_DEFAULTS_IOS_EXCLUDED_PLUGIN_TYPES}")
    if(_lvrs_ios_excluded_plugin_types STREQUAL "" AND LVRS_DEFAULTS_IOS_EXCLUDE_QMLTOOLING)
        set(_lvrs_ios_excluded_plugin_types "qmltooling")
    endif()
    if(NOT _lvrs_ios_excluded_plugin_types STREQUAL "")
        set_property(TARGET "${LVRS_DEFAULTS_TARGET}" PROPERTY
            _LVRS_IOS_EXCLUDED_PLUGIN_TYPES "${_lvrs_ios_excluded_plugin_types}")
    endif()
endfunction()

function(lvrs_configure_qml_app target)
    set(_lvrs_options NO_PLATFORM_RUNTIME_TARGETS)
    cmake_parse_arguments(LVRS_CFG
        "${_lvrs_options}"
        ""
        ""
        ${ARGN}
    )

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "lvrs_configure_qml_app() target not found: ${target}")
    endif()

    target_link_libraries("${target}" PRIVATE LVRS::LVRS)
    _lvrs_internal_apply_safe_default_output_dirs("${target}")
    _lvrs_internal_maybe_link_static_lvrs_plugin("${target}")
    lvrs_apply_platform_build_optimizations("${target}")

    # Allow qmlimportscanner/qmllint and IDE tooling to discover LVRS module
    # metadata while avoiding duplicate LVRS roots (root + module directory).
    set(_lvrs_qml_metadata_root "")
    if(DEFINED LVRS_QML_IMPORT_PATH AND IS_DIRECTORY "${LVRS_QML_IMPORT_PATH}")
        set(_lvrs_qml_metadata_root "${LVRS_QML_IMPORT_PATH}")
    elseif(DEFINED LVRS_QML_MODULE_PATH AND IS_DIRECTORY "${LVRS_QML_MODULE_PATH}")
        # Compatibility fallback for older package layouts.
        set(_lvrs_qml_metadata_root "${LVRS_QML_MODULE_PATH}")
    endif()
    if(NOT _lvrs_qml_metadata_root STREQUAL "")
        _lvrs_internal_append_unique_qml_import_path("${target}" "${_lvrs_qml_metadata_root}")
    endif()
    unset(_lvrs_qml_metadata_root)

    set(_lvrs_is_ios_simulator FALSE)
    if(IOS AND CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
        set(_lvrs_is_ios_simulator TRUE)
    endif()

    # Ensure static QML plugins referenced by LVRS imports are linked/imported
    # when the consuming target is finalized.
    if(COMMAND qt_import_qml_plugins AND LVRS_STATIC_QML_PLUGIN_REQUIRED)
        qt_import_qml_plugins("${target}")
    endif()

    # Some Qt iOS kits ship simulator-incompatible static plugin init objects
    # for these plugin types. Excluding them for simulator builds keeps
    # downstream bootstrap builds runnable without per-project patching.
    if(_lvrs_is_ios_simulator AND COMMAND qt_import_plugins)
        set(_lvrs_ios_excluded_plugin_types "qmltooling;networkinformation;tls;imageformats;iconengines;platforms")
        get_target_property(_lvrs_ios_excluded_plugin_types_target "${target}" _LVRS_IOS_EXCLUDED_PLUGIN_TYPES)
        if(_lvrs_ios_excluded_plugin_types_target
           AND NOT _lvrs_ios_excluded_plugin_types_target STREQUAL "_LVRS_IOS_EXCLUDED_PLUGIN_TYPES-NOTFOUND"
           AND NOT _lvrs_ios_excluded_plugin_types_target STREQUAL "")
            set(_lvrs_ios_excluded_plugin_types "${_lvrs_ios_excluded_plugin_types_target}")
        elseif(DEFINED LVRS_IOS_SIMULATOR_EXCLUDED_PLUGIN_TYPES
           AND NOT LVRS_IOS_SIMULATOR_EXCLUDED_PLUGIN_TYPES STREQUAL "")
            set(_lvrs_ios_excluded_plugin_types "${LVRS_IOS_SIMULATOR_EXCLUDED_PLUGIN_TYPES}")
        elseif(DEFINED ENV{LVRS_IOS_SIMULATOR_EXCLUDED_PLUGIN_TYPES}
               AND NOT "$ENV{LVRS_IOS_SIMULATOR_EXCLUDED_PLUGIN_TYPES}" STREQUAL "")
            set(_lvrs_ios_excluded_plugin_types "$ENV{LVRS_IOS_SIMULATOR_EXCLUDED_PLUGIN_TYPES}")
        endif()
        qt_import_plugins("${target}" EXCLUDE_BY_TYPE ${_lvrs_ios_excluded_plugin_types})

        # If platforms are excluded from Qt's auto-import path, register the
        # iOS platform integration plugin explicitly using a local source file.
        if(_lvrs_ios_excluded_plugin_types MATCHES "(^|;)platforms(;|$)"
           AND TARGET Qt6::QIOSIntegrationPlugin)
            target_link_libraries("${target}" PRIVATE Qt6::QIOSIntegrationPlugin)
            set(_lvrs_ios_platform_plugin_import_source
                "${CMAKE_CURRENT_BINARY_DIR}/${target}_lvrs_ios_platform_plugin_import.cpp")
            file(WRITE "${_lvrs_ios_platform_plugin_import_source}"
                "#include <QtPlugin>\n"
                "Q_IMPORT_PLUGIN(QIOSIntegrationPlugin)\n"
            )
            target_sources("${target}" PRIVATE "${_lvrs_ios_platform_plugin_import_source}")
        endif()
    endif()

    if(NOT LVRS_CFG_NO_PLATFORM_RUNTIME_TARGETS)
        _lvrs_internal_create_platform_runtime_targets("${target}")
        lvrs_add_bootstrap_targets(TARGET "${target}")
    endif()

endfunction()

function(lvrs_add_qml_app)
    set(_lvrs_options NO_PLATFORM_RUNTIME_TARGETS)
    set(_lvrs_one_value_args TARGET URI VERSION ROOT_OBJECT APP_NAME STYLE)
    set(_lvrs_multi_value_args SOURCES QML_FILES RESOURCES)
    cmake_parse_arguments(LVRS_APP
        "${_lvrs_options}"
        "${_lvrs_one_value_args}"
        "${_lvrs_multi_value_args}"
        ${ARGN}
    )

    if(NOT LVRS_APP_TARGET)
        message(FATAL_ERROR "lvrs_add_qml_app() requires TARGET")
    endif()
    if(NOT LVRS_APP_URI)
        message(FATAL_ERROR "lvrs_add_qml_app() requires URI")
    endif()
    if(NOT LVRS_APP_QML_FILES)
        message(FATAL_ERROR "lvrs_add_qml_app() requires at least one QML_FILES entry")
    endif()
    if(NOT TARGET Qt6::Quick OR NOT TARGET Qt6::QuickControls2)
        message(FATAL_ERROR "lvrs_add_qml_app() requires Qt6::Quick and Qt6::QuickControls2 targets. Call find_package(Qt6 ... COMPONENTS Quick QuickControls2) first.")
    endif()

    if(NOT LVRS_APP_VERSION)
        set(LVRS_APP_VERSION "1.0")
    endif()
    if(NOT LVRS_APP_ROOT_OBJECT)
        set(LVRS_APP_ROOT_OBJECT "Main")
    endif()
    if(NOT LVRS_APP_APP_NAME)
        set(LVRS_APP_APP_NAME "${LVRS_APP_TARGET}")
    endif()
    if(NOT LVRS_APP_STYLE)
        set(LVRS_APP_STYLE "Basic")
    endif()

    set(_lvrs_sources ${LVRS_APP_SOURCES})
    if(NOT _lvrs_sources)
        set(LVRS_ENTRY_APP_NAME "${LVRS_APP_APP_NAME}")
        set(LVRS_ENTRY_STYLE "${LVRS_APP_STYLE}")
        set(LVRS_ENTRY_URI "${LVRS_APP_URI}")
        set(LVRS_ENTRY_ROOT_OBJECT "${LVRS_APP_ROOT_OBJECT}")
        set(_lvrs_entry_template "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/LVRSAppEntryPoint.cpp.in")
        if(NOT EXISTS "${_lvrs_entry_template}")
            message(FATAL_ERROR "lvrs_add_qml_app() template not found: ${_lvrs_entry_template}")
        endif()
        set(_lvrs_generated_entry "${CMAKE_CURRENT_BINARY_DIR}/${LVRS_APP_TARGET}_lvrs_entrypoint.cpp")
        configure_file("${_lvrs_entry_template}" "${_lvrs_generated_entry}" @ONLY)
        list(APPEND _lvrs_sources "${_lvrs_generated_entry}")
    endif()

    qt_add_executable(${LVRS_APP_TARGET}
        ${_lvrs_sources}
    )

    set(_lvrs_qml_module_args
        URI ${LVRS_APP_URI}
        VERSION ${LVRS_APP_VERSION}
        RESOURCE_PREFIX "/qt/qml"
        QML_FILES
            ${LVRS_APP_QML_FILES}
    )
    if(LVRS_APP_RESOURCES)
        list(APPEND _lvrs_qml_module_args
            RESOURCES
                ${LVRS_APP_RESOURCES}
        )
    endif()
    qt_add_qml_module(${LVRS_APP_TARGET}
        ${_lvrs_qml_module_args}
    )

    target_link_libraries(${LVRS_APP_TARGET}
        PRIVATE
            Qt6::Quick
            Qt6::QuickControls2
    )

    if(LVRS_APP_NO_PLATFORM_RUNTIME_TARGETS)
        lvrs_configure_qml_app(${LVRS_APP_TARGET} NO_PLATFORM_RUNTIME_TARGETS)
    else()
        lvrs_configure_qml_app(${LVRS_APP_TARGET})
    endif()
endfunction()
