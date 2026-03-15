function(uifw_add_test test_name test_source)
    cmake_parse_arguments(UIFW_TEST "" "" "LABELS" ${ARGN})

    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name}
        PRIVATE
            Qt6::Test
            Qt6::Quick
            Qt6::QuickControls2
            LVRSCore
    )
    target_compile_definitions(${test_name}
        PRIVATE
            LVRS_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    if(NOT LVRS_SANITIZER STREQUAL "none")
        target_compile_options(${test_name}
            PRIVATE
                -fno-omit-frame-pointer
                -fsanitize=${LVRS_SANITIZER}
        )
        target_link_options(${test_name}
            PRIVATE
                -fsanitize=${LVRS_SANITIZER}
        )
    endif()
    if(LVRS_STATIC_QML_PLUGIN_REQUIRED)
        target_link_libraries(${test_name}
            PRIVATE
                LVRSplugin
                LVRSplugin_init
        )
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
    set(_uifw_test_env "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Basic")
    if(UIFW_TEST_LABELS)
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT "${_uifw_test_env}"
            LABELS "${UIFW_TEST_LABELS}"
        )
    else()
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT "${_uifw_test_env}"
        )
    endif()
endfunction()

add_test(
    NAME LVRSTests_cmake_bootstrap_lvrs_dir_resolution
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        "-DLVRS_BINARY_DIR=${CMAKE_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_bootstrap_lvrs_dir_resolution.cmake"
)

add_test(
    NAME LVRSTests_cmake_example_source_snapshot_runtime_path
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_example_source_snapshot_runtime_path.cmake"
)

add_test(
    NAME LVRSTests_cmake_example_runtime_output_dir
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_example_runtime_output_dir.cmake"
)

add_test(
    NAME LVRSTests_cmake_example_host_aggregate_target
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_BINARY_DIR=${CMAKE_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_example_host_aggregate_target.cmake"
)

add_test(
    NAME LVRSTests_cmake_app_bootstrap_skip_missing_qt_kit
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        "-DLVRS_BINARY_DIR=${CMAKE_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_app_bootstrap_skip_missing_qt_kit.cmake"
)

add_test(
    NAME LVRSTests_cmake_linux_runtime_bundle_helper
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        "-DLVRS_BINARY_DIR=${CMAKE_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_linux_runtime_bundle_helper.cmake"
)

uifw_add_test(LVRSTests_layout tst_layout.cpp)
uifw_add_test(LVRSTests_app_entry tst_app_entry.cpp)
uifw_add_test(LVRSTests_eventlistener tst_eventlistener.cpp)
uifw_add_test(LVRSTests_examples tst_examples.cpp)
uifw_add_test(LVRSTests_debug tst_debug.cpp)
uifw_add_test(LVRSTests_import_api tst_import_api.cpp)
uifw_add_test(LVRSTests_runtimeevents tst_runtimeevents.cpp)
uifw_add_test(LVRSTests_backend_io tst_backend_io.cpp)
uifw_add_test(LVRSTests_app_state tst_app_state.cpp)
uifw_add_test(LVRSTests_render_quality tst_render_quality.cpp)
uifw_add_test(LVRSTests_font_policy tst_font_policy.cpp)
uifw_add_test(LVRSTests_navigation_state tst_navigation_state.cpp)
uifw_add_test(LVRSTests_runtime_services tst_runtime_services.cpp)
uifw_add_test(LVRSTests_svg_manager tst_svg_manager.cpp)
uifw_add_test(LVRSTests_platform_integration tst_platform_integration.cpp)
uifw_add_test(LVRSTests_page_router tst_page_router.cpp)
uifw_add_test(LVRSTests_code_editor tst_code_editor.cpp)
uifw_add_test(LVRSTests_text_editor tst_text_editor.cpp)
uifw_add_test(LVRSTests_text_markup tst_text_markup.cpp)
uifw_add_test(LVRSTests_hierarchy_codec tst_hierarchy_codec.cpp)
uifw_add_test(LVRSTests_hierarchy_interaction tst_hierarchy_interaction.cpp)
uifw_add_test(LVRSTests_performance_gate tst_performance_gate.cpp LABELS "p4;quality;performance;ci")
uifw_add_test(LVRSTests_visual_regression tst_visual_regression.cpp LABELS "p4;quality;visual;ci")
uifw_add_test(LVRSTests_soak_runtime tst_soak_runtime.cpp LABELS "p4;quality;soak;long")
