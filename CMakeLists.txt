cmake_minimum_required(VERSION 3.21)
project(LVRS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(LVRS_DEFAULT_BUILD_EXAMPLES ON)
    set(LVRS_DEFAULT_INSTALL_EXAMPLES ON)
else()
    # When LVRS is pulled as a subproject (e.g., standalone example builds),
    # default to library-only to avoid recursive example target definitions.
    set(LVRS_DEFAULT_BUILD_EXAMPLES OFF)
    set(LVRS_DEFAULT_INSTALL_EXAMPLES OFF)
endif()

option(LVRS_BUILD_SHARED_LIBS "Build LVRS as shared library for dynamic consumption" ON)
option(LVRS_BUILD_EXAMPLES "Build runnable example applications" ${LVRS_DEFAULT_BUILD_EXAMPLES})
option(LVRS_INSTALL_EXAMPLES "Install example application binaries when examples are built" ${LVRS_DEFAULT_INSTALL_EXAMPLES})
option(LVRS_BUILD_TESTS "Build LVRS tests" OFF)
option(LVRS_INSTALL_QML_MODULE "Install LVRS QML module directory for downstream imports" ON)
option(LVRS_ENABLE_FRAMEWORK_BOOTSTRAP_TARGETS
    "Generate bootstrap_lvrs_* targets that build/install LVRS for all runtime platforms"
    ON
)

option(LVRS_FORCE_X86_QT_TOOLS "Force Qt host tools to run under x86_64 on Apple hosts" OFF)
option(LVRS_AUTO_FALLBACK_X86_QT_TOOLS
    "On Apple hosts, fallback Qt host tools to x86_64 when native arm64 tool execution fails"
    ON
)
option(LVRS_ENABLE_APPLE_GL_LINK_SHIM "Provide a minimal AGL shim only for Apple linker compatibility" ON)
option(LVRS_ENFORCE_VULKAN
    "Fail configure if required fixed graphics backend Qt features are unavailable"
    ON
)
option(LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS
    "Apply platform-specific release optimization flags (compile/link) to LVRS targets"
    ON
)
option(LVRS_ENABLE_IPO
    "Enable interprocedural optimization (LTO) for release-like configurations when supported"
    ON
)

unset(LVRS_DEFAULT_BUILD_EXAMPLES)
unset(LVRS_DEFAULT_INSTALL_EXAMPLES)

set(LVRS_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/LVRS")
set(LVRS_QML_MODULE_INSTALL_BASE "${CMAKE_INSTALL_LIBDIR}/qt6/qml")
set(LVRS_QML_MODULE_INSTALL_DIR "${LVRS_QML_MODULE_INSTALL_BASE}/LVRS")
set(LVRS_APPLE_FRAMEWORK_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}")
set(LVRS_QML_IMPORT_PATH "${CMAKE_CURRENT_BINARY_DIR}")
set(LVRS_QML_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/LVRS")
set(LVRS_RUNTIME_PLATFORMS
    macos
    linux
    windows
    ios
    android
    wasm
)
set(LVRS_BOOTSTRAP_INSTALL_ROOT
    "${CMAKE_BINARY_DIR}/lvrs-install"
    CACHE PATH
    "Root directory where bootstrap_lvrs_* installs per-platform LVRS packages"
)
set(LVRS_APPLE_AGL_SHIM_ACTIVE OFF)
set(LVRS_APPLE_AGL_FRAMEWORK_ROOT "")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/LVRSHelpers.cmake)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Qml Svg Network)
if(LVRS_BUILD_TESTS)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
endif()
find_package(Vulkan QUIET)

if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

qt_standard_project_setup()

function(_lvrs_detect_qt_feature feature_name out_value out_header)
    set(_lvrs_qtgui_config_header "")
    if(DEFINED Qt6Gui_DIR)
        get_filename_component(_lvrs_qt_prefix "${Qt6Gui_DIR}/../../.." ABSOLUTE)
        get_filename_component(_lvrs_qt_lib_dir "${Qt6Gui_DIR}/../.." ABSOLUTE)
        set(_lvrs_qtgui_config_candidates
            "${_lvrs_qt_lib_dir}/QtGui.framework/Headers/qtgui-config.h"
            "${_lvrs_qt_prefix}/include/QtGui/qtgui-config.h"
            "${_lvrs_qt_prefix}/include/QtGui.framework/Headers/qtgui-config.h"
        )
        foreach(_lvrs_candidate_header IN LISTS _lvrs_qtgui_config_candidates)
            if(EXISTS "${_lvrs_candidate_header}")
                set(_lvrs_qtgui_config_header "${_lvrs_candidate_header}")
                break()
            endif()
        endforeach()
        unset(_lvrs_qtgui_config_candidates)
        unset(_lvrs_candidate_header)
        unset(_lvrs_qt_prefix)
        unset(_lvrs_qt_lib_dir)
    endif()

    if(NOT _lvrs_qtgui_config_header)
        set(${out_value} "" PARENT_SCOPE)
        set(${out_header} "" PARENT_SCOPE)
        return()
    endif()

    string(TOLOWER "${feature_name}" _lvrs_feature_name)
    file(STRINGS "${_lvrs_qtgui_config_header}" _lvrs_feature_line REGEX "^#define QT_FEATURE_${_lvrs_feature_name} ")
    if(_lvrs_feature_line MATCHES "(-?[0-9]+)$")
        set(_lvrs_feature_value "${CMAKE_MATCH_1}")
    else()
        set(_lvrs_feature_value "")
    endif()

    set(${out_value} "${_lvrs_feature_value}" PARENT_SCOPE)
    set(${out_header} "${_lvrs_qtgui_config_header}" PARENT_SCOPE)
endfunction()

set(LVRS_FORCED_BACKEND "default")
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(LVRS_FORCED_BACKEND "metal")
elseif(WIN32 OR ANDROID)
    set(LVRS_FORCED_BACKEND "vulkan")
endif()

if(APPLE AND NOT IOS AND LVRS_ENABLE_APPLE_GL_LINK_SHIM)
    if(NOT EXISTS "/System/Library/Frameworks/AGL.framework/AGL")
        set(_uifw_framework_root "${CMAKE_CURRENT_BINARY_DIR}/frameworks")
        set(_uifw_agl_framework "${_uifw_framework_root}/AGL.framework")
        set(_uifw_agl_versions "${_uifw_agl_framework}/Versions")
        set(_uifw_agl_version_a "${_uifw_agl_versions}/A")
        set(_uifw_agl_binary "${_uifw_agl_version_a}/AGL")

        file(MAKE_DIRECTORY "${_uifw_agl_version_a}/Headers")
        file(MAKE_DIRECTORY "${_uifw_agl_version_a}/Resources")
        set(_uifw_agl_rebuild FALSE)
        if(EXISTS "${_uifw_agl_binary}")
            execute_process(
                COMMAND lipo -info "${_uifw_agl_binary}"
                OUTPUT_VARIABLE _uifw_agl_lipo_info
                RESULT_VARIABLE _uifw_agl_lipo_result
            )
            if(NOT _uifw_agl_lipo_result EQUAL 0)
                set(_uifw_agl_rebuild TRUE)
            elseif(CMAKE_OSX_ARCHITECTURES)
                foreach(_uifw_arch IN LISTS CMAKE_OSX_ARCHITECTURES)
                    if(NOT _uifw_agl_lipo_info MATCHES "${_uifw_arch}")
                        set(_uifw_agl_rebuild TRUE)
                    endif()
                endforeach()
            endif()
        else()
            set(_uifw_agl_rebuild TRUE)
        endif()

        if(_uifw_agl_rebuild)
            set(_uifw_agl_stub "${CMAKE_CURRENT_BINARY_DIR}/agl_stub.cpp")
            file(WRITE "${_uifw_agl_stub}" "void _uifw_dummy_agl(void) {}\n")
            set(_uifw_agl_arch_flags "")
            if(CMAKE_OSX_ARCHITECTURES)
                foreach(_uifw_arch IN LISTS CMAKE_OSX_ARCHITECTURES)
                    list(APPEND _uifw_agl_arch_flags -arch "${_uifw_arch}")
                endforeach()
            endif()
            execute_process(
                COMMAND "${CMAKE_CXX_COMPILER}" ${_uifw_agl_arch_flags} -dynamiclib "${_uifw_agl_stub}" -install_name "@rpath/AGL.framework/Versions/A/AGL" -o "${_uifw_agl_binary}"
                RESULT_VARIABLE _uifw_agl_build_result
            )
            if(NOT _uifw_agl_build_result EQUAL 0)
                message(FATAL_ERROR "Failed to build Apple GL link shim")
            endif()
            file(WRITE "${_uifw_agl_version_a}/Headers/AGL.h" "void _uifw_dummy_agl(void);\n")
            file(WRITE "${_uifw_agl_version_a}/Resources/Info.plist" "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n<plist version=\"1.0\"><dict><key>CFBundleName</key><string>AGL</string></dict></plist>\n")
        endif()
        if(NOT EXISTS "${_uifw_agl_versions}/Current")
            file(CREATE_LINK "A" "${_uifw_agl_versions}/Current" SYMBOLIC)
        endif()
        if(EXISTS "${_uifw_agl_framework}/AGL" OR IS_SYMLINK "${_uifw_agl_framework}/AGL")
            file(REMOVE "${_uifw_agl_framework}/AGL")
        endif()
        file(CREATE_LINK "Versions/Current/AGL" "${_uifw_agl_framework}/AGL" SYMBOLIC)
        if(EXISTS "${_uifw_agl_framework}/Headers" OR IS_SYMLINK "${_uifw_agl_framework}/Headers")
            file(REMOVE "${_uifw_agl_framework}/Headers")
        endif()
        if(EXISTS "${_uifw_agl_framework}/Headers")
            file(REMOVE_RECURSE "${_uifw_agl_framework}/Headers")
        endif()
        file(CREATE_LINK "Versions/Current/Headers" "${_uifw_agl_framework}/Headers" SYMBOLIC)
        if(EXISTS "${_uifw_agl_framework}/Resources" OR IS_SYMLINK "${_uifw_agl_framework}/Resources")
            file(REMOVE "${_uifw_agl_framework}/Resources")
        endif()
        if(EXISTS "${_uifw_agl_framework}/Resources")
            file(REMOVE_RECURSE "${_uifw_agl_framework}/Resources")
        endif()
        file(CREATE_LINK "Versions/Current/Resources" "${_uifw_agl_framework}/Resources" SYMBOLIC)
        set(LVRS_APPLE_AGL_SHIM_ACTIVE ON)
        set(LVRS_APPLE_AGL_FRAMEWORK_ROOT "${_uifw_framework_root}")
        set(CMAKE_FRAMEWORK_PATH "${_uifw_framework_root}" ${CMAKE_FRAMEWORK_PATH})
        add_link_options("-F${_uifw_framework_root}")
    endif()
endif()

if(APPLE)
    function(_lvrs_resolve_qt_tool_realpath target out_path)
        if(NOT TARGET ${target})
            set(${out_path} "" PARENT_SCOPE)
            return()
        endif()

        get_target_property(_lvrs_imported_configs ${target} IMPORTED_CONFIGURATIONS)
        set(_lvrs_location_props IMPORTED_LOCATION)
        if(_lvrs_imported_configs)
            foreach(_lvrs_config IN LISTS _lvrs_imported_configs)
                string(TOUPPER "${_lvrs_config}" _lvrs_config_upper)
                list(APPEND _lvrs_location_props "IMPORTED_LOCATION_${_lvrs_config_upper}")
            endforeach()
        endif()

        foreach(_lvrs_location_prop IN LISTS _lvrs_location_props)
            get_target_property(_lvrs_tool_path ${target} ${_lvrs_location_prop})
            if(NOT _lvrs_tool_path)
                continue()
            endif()
            get_filename_component(_lvrs_tool_realpath "${_lvrs_tool_path}" REALPATH)
            if(EXISTS "${_lvrs_tool_realpath}")
                set(${out_path} "${_lvrs_tool_realpath}" PARENT_SCOPE)
                return()
            endif()
        endforeach()

        set(${out_path} "" PARENT_SCOPE)
    endfunction()

    function(_lvrs_qt_tool_arch_works target arch out_ok)
        _lvrs_resolve_qt_tool_realpath("${target}" _lvrs_tool_realpath)
        if(NOT _lvrs_tool_realpath)
            set(${out_ok} OFF PARENT_SCOPE)
            return()
        endif()

        execute_process(
            COMMAND arch "-${arch}" "${_lvrs_tool_realpath}" --version
            RESULT_VARIABLE _lvrs_tool_result
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(_lvrs_tool_result EQUAL 0)
            set(${out_ok} ON PARENT_SCOPE)
        else()
            set(${out_ok} OFF PARENT_SCOPE)
        endif()
    endfunction()

    set(_lvrs_wrap_qt_tools_x86 OFF)
    if(LVRS_FORCE_X86_QT_TOOLS)
        set(_lvrs_wrap_qt_tools_x86 ON)
        message(STATUS "LVRS_FORCE_X86_QT_TOOLS is ON: wrapping Qt host tools with arch -x86_64.")
    elseif(LVRS_AUTO_FALLBACK_X86_QT_TOOLS)
        _lvrs_qt_tool_arch_works(Qt6::rcc arm64 _lvrs_rcc_arm64_works)
        _lvrs_qt_tool_arch_works(Qt6::rcc x86_64 _lvrs_rcc_x86_works)
        if(NOT _lvrs_rcc_arm64_works AND _lvrs_rcc_x86_works)
            set(_lvrs_wrap_qt_tools_x86 ON)
            message(WARNING
                "Qt host tools fail natively on arm64 (for example, rcc --version). "
                "Enabling automatic x86_64 fallback wrappers."
            )
        endif()
    endif()

    if(_lvrs_wrap_qt_tools_x86)
        function(_uifw_wrap_qt_tool target)
            if(NOT TARGET ${target})
                return()
            endif()

            get_target_property(_uifw_imported_configs ${target} IMPORTED_CONFIGURATIONS)
            set(_uifw_location_props IMPORTED_LOCATION)
            if(_uifw_imported_configs)
                foreach(_uifw_config IN LISTS _uifw_imported_configs)
                    string(TOUPPER "${_uifw_config}" _uifw_config_upper)
                    list(APPEND _uifw_location_props "IMPORTED_LOCATION_${_uifw_config_upper}")
                endforeach()
            endif()

            string(REPLACE "::" "_" _uifw_tool_name "${target}")
            set(_uifw_wrapped_any OFF)

            foreach(_uifw_location_prop IN LISTS _uifw_location_props)
                get_target_property(_uifw_tool_path ${target} ${_uifw_location_prop})
                if(NOT _uifw_tool_path)
                    continue()
                endif()
                get_filename_component(_uifw_tool_realpath "${_uifw_tool_path}" REALPATH)
                if(NOT EXISTS "${_uifw_tool_realpath}")
                    continue()
                endif()

                string(TOLOWER "${_uifw_location_prop}" _uifw_prop_suffix)
                set(_uifw_wrapper "${CMAKE_CURRENT_BINARY_DIR}/${_uifw_tool_name}_${_uifw_prop_suffix}_x86_wrapper")
                file(WRITE "${_uifw_wrapper}" "#!/bin/sh\nexec arch -x86_64 \"${_uifw_tool_realpath}\" \"$@\"\n")
                file(CHMOD "${_uifw_wrapper}"
                    PERMISSIONS
                        OWNER_READ OWNER_WRITE OWNER_EXECUTE
                        GROUP_READ GROUP_EXECUTE
                        WORLD_READ WORLD_EXECUTE
                )
                set_property(TARGET ${target} PROPERTY ${_uifw_location_prop} "${_uifw_wrapper}")
                set(_uifw_wrapped_any ON)
            endforeach()

            if(NOT _uifw_wrapped_any)
                message(WARNING "Qt host tool wrapping is enabled, but ${target} had no import location to wrap.")
            endif()
        endfunction()

        _uifw_wrap_qt_tool(Qt6::rcc)
        if(TARGET Qt6::qmlcachegen)
            _uifw_wrap_qt_tool(Qt6::qmlcachegen)
        endif()
        if(TARGET Qt6::qmlsc)
            _uifw_wrap_qt_tool(Qt6::qmlsc)
        endif()
        if(TARGET Qt6::qmltyperegistrar)
            _uifw_wrap_qt_tool(Qt6::qmltyperegistrar)
        endif()
        if(TARGET Qt6::uic)
            _uifw_wrap_qt_tool(Qt6::uic)
        endif()
        if(TARGET Qt6::qmlimportscanner)
            _uifw_wrap_qt_tool(Qt6::qmlimportscanner)
        endif()
    endif()
endif()

if(LVRS_BUILD_SHARED_LIBS)
    set(_lvrs_library_type SHARED)
else()
    set(_lvrs_library_type STATIC)
endif()

qt_add_library(LVRSCore ${_lvrs_library_type})
set_target_properties(LVRSCore PROPERTIES
    OUTPUT_NAME "LVRS"
    EXPORT_NAME "LVRS"
)
add_library(LVRS::LVRS ALIAS LVRSCore)

if(LVRS_APPLE_AGL_SHIM_ACTIVE)
    target_link_options(LVRSCore INTERFACE
        $<BUILD_INTERFACE:-F${LVRS_APPLE_AGL_FRAMEWORK_ROOT}>
        $<BUILD_INTERFACE:-Wl,-rpath,${LVRS_APPLE_AGL_FRAMEWORK_ROOT}>
        $<INSTALL_INTERFACE:-F$<INSTALL_PREFIX>/${LVRS_APPLE_FRAMEWORK_INSTALL_DIR}>
        $<INSTALL_INTERFACE:-Wl,-rpath,$<INSTALL_PREFIX>/${LVRS_APPLE_FRAMEWORK_INSTALL_DIR}>
    )
endif()

set(LVRS_STATIC_QML_PLUGIN_REQUIRED OFF)
if(NOT LVRS_BUILD_SHARED_LIBS)
    set(LVRS_STATIC_QML_PLUGIN_REQUIRED ON)
    target_compile_definitions(LVRSCore INTERFACE LVRS_USE_STATIC_QML_PLUGIN=1)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/backend/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/qml/CMakeLists.txt)

target_include_directories(LVRSCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/backend>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/backend/io>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/backend/fonts>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/backend/platform>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/backend/runtime>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/backend/navigation>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/backend/state>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/backend/graphics>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/backend/text>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/LVRS>
)

qt_add_qml_module(LVRSCore
    URI LVRS
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/LVRS"
    SOURCES
        ${LVRS_BACKEND_SOURCES}
        ${LVRS_BACKEND_HEADERS}
    QML_FILES
        ${LVRS_QML_FILES}
    RESOURCES
        ${LVRS_RESOURCE_FILES}
)

# LVRSCore is a library target; QML plugin import scanning is performed on
# consuming app targets via lvrs_configure_qml_app().
set_target_properties(LVRSCore PROPERTIES
    QT_QML_MODULE_NO_IMPORT_SCAN TRUE
)

lvrs_apply_platform_build_optimizations(LVRSCore)
if(TARGET LVRSCoreplugin)
    lvrs_apply_platform_build_optimizations(LVRSCoreplugin)
endif()

if(TARGET LVRSCoreplugin)
    add_library(LVRSplugin ALIAS LVRSCoreplugin)
endif()
if(TARGET LVRSCoreplugin_init)
    add_library(LVRSplugin_init ALIAS LVRSCoreplugin_init)
endif()

if(LVRS_ENFORCE_VULKAN)
    if(LVRS_FORCED_BACKEND STREQUAL "vulkan")
        if(NOT TARGET Vulkan::Vulkan)
            message(WARNING
                "LVRS_ENFORCE_VULKAN=ON: Vulkan::Vulkan was not discovered at configure time. "
                "LVRS will continue because runtime Vulkan loader discovery is handled dynamically "
                "in backend/runtime/vulkanbootstrap.cpp.")
        endif()

        _lvrs_detect_qt_feature(vulkan _lvrs_qt_vulkan_feature _lvrs_qtgui_config_header)
        if(_lvrs_qt_vulkan_feature STREQUAL "")
            message(FATAL_ERROR
                "LVRS_ENFORCE_VULKAN=ON but QT_FEATURE_vulkan could not be resolved from qtgui-config.h.")
        endif()

        if(_lvrs_qt_vulkan_feature LESS 0)
            message(FATAL_ERROR
                "LVRS_ENFORCE_VULKAN=ON but this Qt build has QT_FEATURE_vulkan=${_lvrs_qt_vulkan_feature} "
                "(${_lvrs_qtgui_config_header}). Install/rebuild Qt with Vulkan support.")
        endif()
    elseif(LVRS_FORCED_BACKEND STREQUAL "metal")
        _lvrs_detect_qt_feature(metal _lvrs_qt_metal_feature _lvrs_qtgui_config_header)
        if(_lvrs_qt_metal_feature STREQUAL "")
            message(FATAL_ERROR
                "LVRS_ENFORCE_VULKAN=ON but QT_FEATURE_metal could not be resolved from qtgui-config.h.")
        endif()

        if(_lvrs_qt_metal_feature LESS 0)
            message(FATAL_ERROR
                "LVRS_ENFORCE_VULKAN=ON and this platform requires Metal, but this Qt build has QT_FEATURE_metal=${_lvrs_qt_metal_feature} "
                "(${_lvrs_qtgui_config_header}). Install/rebuild Qt with Metal support.")
        endif()
    endif()
endif()

target_link_libraries(LVRSCore
    PUBLIC
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Qml
        Qt6::Svg
        Qt6::Network
)

if(LVRS_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

if(LVRS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(LVRS_ENABLE_FRAMEWORK_BOOTSTRAP_TARGETS)
    lvrs_create_framework_bootstrap_targets()
endif()

if(LVRS_BUILD_SHARED_LIBS)
    install(TARGETS LVRSCore
        EXPORT LVRSTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/LVRS
    )

    if(LVRS_INSTALL_QML_MODULE AND TARGET LVRSCoreplugin)
        install(TARGETS LVRSCoreplugin
            EXPORT LVRSTargets
            ARCHIVE DESTINATION ${LVRS_QML_MODULE_INSTALL_DIR}
            LIBRARY DESTINATION ${LVRS_QML_MODULE_INSTALL_DIR}
            RUNTIME DESTINATION ${LVRS_QML_MODULE_INSTALL_DIR}
        )
    endif()

    install(EXPORT LVRSTargets
        NAMESPACE LVRS::
        DESTINATION ${LVRS_INSTALL_CMAKE_DIR}
    )
else()
    install(TARGETS LVRSCore
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/LVRS
    )

    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/cmake/LVRSTargetsStatic.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/LVRSTargets.cmake"
        @ONLY
    )
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/LVRSTargets.cmake"
        DESTINATION ${LVRS_INSTALL_CMAKE_DIR}
    )
endif()

install(
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/backend/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/LVRS/backend
    FILES_MATCHING
        PATTERN "*.h"
)

if(LVRS_INSTALL_QML_MODULE)
    install(
        DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/LVRS/"
        DESTINATION "${LVRS_QML_MODULE_INSTALL_DIR}"
        PATTERN "*LVRSCoreplugin*" EXCLUDE
    )
endif()

if(LVRS_APPLE_AGL_SHIM_ACTIVE)
    install(
        DIRECTORY "${LVRS_APPLE_AGL_FRAMEWORK_ROOT}/AGL.framework"
        DESTINATION "${LVRS_APPLE_FRAMEWORK_INSTALL_DIR}"
    )
endif()

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/LVRSConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/LVRSConfig.cmake"
    INSTALL_DESTINATION ${LVRS_INSTALL_CMAKE_DIR}
    PATH_VARS
        LVRS_QML_MODULE_INSTALL_BASE
        LVRS_QML_MODULE_INSTALL_DIR
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/LVRSConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/LVRSConfigRoot.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/LVRSConfigRoot.cmake"
    @ONLY
)

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/LVRSConfigVersionRoot.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/LVRSConfigVersionRoot.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/LVRSConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/LVRSConfigVersion.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/cmake/LVRSHelpers.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/cmake/LVRSBootstrapAction.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/cmake/LVRSBootstrapExportAction.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/cmake/LVRSBootstrapWasmLaunchAction.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/cmake/LVRSBootstrapFrameworkAction.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/cmake/LVRSAppEntryPoint.cpp.in"
    DESTINATION ${LVRS_INSTALL_CMAKE_DIR}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/LVRSConfigRoot.cmake"
    DESTINATION "."
    RENAME "LVRSConfig.cmake"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/LVRSConfigVersionRoot.cmake"
    DESTINATION "."
    RENAME "LVRSConfigVersion.cmake"
)
