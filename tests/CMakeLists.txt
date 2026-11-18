function(uifw_add_test test_name test_source)
    cmake_parse_arguments(UIFW_TEST "" "" "LABELS" ${ARGN})

    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name}
        PRIVATE
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
    # Test the library and QML produced by this build even after sourcing an
    # installed SDK's env.sh. An inherited loader path can otherwise silently
    # replace LVRSCore (including its embedded QML) with an older installation.
    if(WIN32)
        set(_uifw_qpa windows) # Exercise the native Windows font engine.
    else()
        set(_uifw_qpa offscreen)
    endif()
    set(_uifw_test_env
        "QT_QPA_PLATFORM=${_uifw_qpa}"
        "QT_QUICK_CONTROLS_STYLE=Basic"
        "QML_IMPORT_PATH=${CMAKE_BINARY_DIR}"
        "QML2_IMPORT_PATH=${CMAKE_BINARY_DIR}"
    )
    if(APPLE)
        list(APPEND _uifw_test_env "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:LVRSCore>")
    elseif(UNIX)
        list(APPEND _uifw_test_env "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:LVRSCore>")
    endif()
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

uifw_add_test(LVRSTests_materials tst_materials.cpp)
set_tests_properties(LVRSTests_materials PROPERTIES SKIP_RETURN_CODE 77)
get_test_property(LVRSTests_materials ENVIRONMENT _materials_test_environment)
foreach(case IN ITEMS
        window_default_and_primary
        window_material_inherits_owning_window_accent
        application_window_theme_accent_mix
        application_window_custom_background
        noise_texture_defaults_match_figma
        noise_texture_inputs_are_bounded_and_opaque
        window_gradient_tracks_accent_and_resize
        transient_defaults_and_capture_safety
        backdrop_blur_and_color_rendering
        catalog_transient_interaction
        context_menu_frost_contract
        context_menu_frost_rendering
    )
    add_test(NAME "LVRSTests_materials.${case}" COMMAND LVRSTests_materials "${case}")
    set_tests_properties("LVRSTests_materials.${case}" PROPERTIES
        ENVIRONMENT "${_materials_test_environment}" TIMEOUT 60 SKIP_RETURN_CODE 77)
endforeach()
if(WIN32)
    uifw_add_test(LVRSTests_windows_exports tst_windows_exports.cpp)
    uifw_add_test(LVRSTests_windows_qml_link tst_windows_qml_link.cpp)
endif()
if(APPLE AND NOT IOS)
    uifw_add_test(LVRSTests_nativewindowblur tst_nativewindowblur.mm)
    set_tests_properties(LVRSTests_nativewindowblur PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(LVRSTests_nativewindowblur PRIVATE "-framework AppKit")
endif()

if(APPLE AND NOT CMAKE_CROSSCOMPILING)
    add_test(
        NAME LVRSTests_cmake_swift_release_optimizations
        COMMAND "${CMAKE_COMMAND}"
            "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
            "-DLVRS_BINARY_DIR=${CMAKE_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_swift_release_optimizations.cmake"
    )
endif()

add_test(
    NAME LVRSTests_cmake_bootstrap_lvrs_dir_resolution
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        "-DLVRS_BINARY_DIR=${CMAKE_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_bootstrap_lvrs_dir_resolution.cmake"
)

add_test(
    NAME LVRSTests_cmake_bootstrap_osx_deployment_target_resolution
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_bootstrap_osx_deployment_target_resolution.cmake"
)

add_test(
    NAME LVRSTests_cmake_example_source_snapshot_runtime_path
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_example_source_snapshot_runtime_path.cmake"
)

add_test(
    NAME LVRSTests_cmake_example_build_tree_runtime_path
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_example_build_tree_runtime_path.cmake"
)

add_test(
    NAME LVRSTests_cmake_build_script_contract
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_build_script_contract.cmake"
)

add_test(
    NAME LVRSTests_cmake_framework_bootstrap_action_contract
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_framework_bootstrap_action_contract.cmake"
)
add_test(NAME LVRSTests_cmake_framework_provider_bootstrap
    COMMAND "${CMAKE_COMMAND}" "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        "-DLVRS_BINARY_DIR=${CMAKE_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_framework_provider_bootstrap.cmake")
add_test(NAME LVRSTests_cmake_android_ndk_cpp23
    COMMAND "${CMAKE_COMMAND}" "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        "-DLVRS_BINARY_DIR=${CMAKE_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_android_ndk_cpp23.cmake")

add_test(
    NAME LVRSTests_cmake_example_runtime_output_dir
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_example_runtime_output_dir.cmake"
)

if(LVRS_BUILD_EXAMPLES)
add_test(
    NAME LVRSTests_cmake_example_host_aggregate_target
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_BINARY_DIR=${CMAKE_BINARY_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_example_host_aggregate_target.cmake"
)
endif()

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
uifw_add_test(LVRSTests_bootstrap_parallel tst_bootstrap_parallel.cpp)
uifw_add_test(LVRSTests_foreground_services tst_foreground_services.cpp)
uifw_add_test(LVRSTests_permission_sequencer tst_permission_sequencer.cpp)
uifw_add_test(LVRSTests_app_entry tst_app_entry.cpp)
uifw_add_test(LVRSTests_eventlistener tst_eventlistener.cpp)
uifw_add_test(LVRSTests_examples tst_examples.cpp)
uifw_add_test(LVRSTests_ctest_event_support ctest_event_support.cpp)
uifw_add_test(LVRSTests_catalog tst_catalog.cpp)
set_tests_properties(LVRSTests_catalog PROPERTIES SKIP_RETURN_CODE 77)
uifw_add_test(LVRSTests_debug tst_debug.cpp)
uifw_add_test(LVRSTests_import_api tst_import_api.cpp)
set_tests_properties(LVRSTests_import_api PROPERTIES SKIP_RETURN_CODE 77)
get_test_property(LVRSTests_import_api ENVIRONMENT _import_api_test_environment)
foreach(case IN ITEMS
        embedded_qml_matches_source
        app_bootstrap_window_loads
        versionless_import_application_window_loads
        application_window_mobile_coverage_visibility_contract_loads
        application_window_page_stack_state_loads
        application_window_initial_properties_seed_page_stack
        application_window_safe_margin_scopes_to_layout_not_render_surface
        versionless_import_window_loads
        window_custom_chrome_interaction_contract_loads
        window_safe_margin_scopes_to_layout_not_render_surface
        appshell_compat_loads
        application_window_platform_adaptive_layout_loads
        icon_name_mapping_loads
        compact_icon_frame_contract_loads
        hierarchy_tree_model_api_loads
        hierarchy_string_array_model_loads
        hierarchy_direct_model_contract_loads
        hierarchy_direct_model_editing_contract_loads
        hierarchy_nested_children_indent_contract_loads
        hierarchy_expansion_uses_incremental_visibility_refresh
        hierarchy_editable_drag_depth_contract_loads
        hierarchy_editable_drag_per_item_lock_contract_loads
        hierarchy_mobile_drag_hold_contract_loads
        hierarchy_mobile_activation_commits_on_release_contract_loads
        hierarchy_mobile_reactivation_reemits_active_signal
        hierarchy_mobile_scroll_physics_contract_loads
        hierarchy_optional_footer_contract_loads
        hierarchy_toolbar_empty_state_does_not_reschedule
        hierarchy_toolbar_item_model_contract_loads
        hierarchy_toolbar_figma_layout_contract_loads
        hierarchy_figma_composition_contract_loads
        hierarchy_toolbar_manual_icon_button_contract_loads
        hierarchy_row_click_only_activates_not_toggles
        hierarchy_chevron_requires_children_loads
        hierarchy_item_chevron_direction_contract_loads
        hierarchy_item_hover_and_active_state_visual_contract_loads
        hierarchy_item_figma_defaults_contract_loads
        hierarchy_item_layout_geometry_contract_loads
        hierarchy_item_count_view_contract_loads
        hierarchy_item_structure_api_contract_loads
        hierarchy_item_ux_state_contract_loads
        button_padding_matches_figma_spec
        button_default_tone_matches_figma_accent_loads
        segmented_button_selected_color
        button_injected_methods_contract_loads
        stepper_figma_contract_loads
        combo_box_figma_contract_loads
        input_field_figma_contract_loads
        input_field_material_rendering_contract
        progress_bar_range_contract_loads
        control_icons_use_supersampled_raster_contract
        input_field_ios_native_text_interaction_contract_loads
        input_field_native_event_input_contract_loads
        input_field_search_icon_mobile_scaling_contract_loads
        toggle_switch_figma_contract_loads
        checkbox_figma_contract_loads
        radio_button_figma_contract_loads
        modal_empty_frame_contract_loads
        modal_content_action_contract_loads
        alert_figma_variant_contract_loads
        alert_content_arguments_contract_loads
        alert_action_button_padding_scopes_to_alert
        menu_item_key_and_chevron_contract_loads
        menu_item_icon_slot_switch_contract_loads
        context_menu_item_action_contract_loads
        context_menu_visual_contract_loads
        context_menu_icon_slot_switch_contract_loads
        context_menu_width_expansion_contract_loads
        context_menu_auto_placement_contract_loads
        model_component_delegate_contract_loads
        table_figma_geometry_contract_loads
        table_spreadsheet_api_contract_loads
        table_viewport_reuses_delegates_and_creates_one_menu_on_demand
        table_cell_item_contract_loads
        table_cell_merge_split_contract_loads
        table_structure_editing_contract_loads
        table_resize_contract_loads
        table_typed_header_contract_loads
        table_cpp_model_contract_loads
        table_undo_redo_contract_loads
        list_model_contract_loads
        list_figma_contract_loads
        button_family_components_contract:desktop
        button_family_components_contract:mobile
        segmented_control_figma_contract_loads:desktop
        segmented_control_figma_contract_loads:mobile
        alert_button_methods_are_invoked:single-primary
        alert_button_methods_are_invoked:horizontal-primary
        alert_button_methods_are_invoked:horizontal-secondary
        alert_button_methods_are_invoked:vertical-primary
        alert_button_methods_are_invoked:vertical-secondary
        alert_button_methods_are_invoked:vertical-tertiary
        alert_glass_overlay_and_input_contract:qt-window
        alert_glass_overlay_and_input_contract:lvrs-window
)
    add_test(NAME "LVRSTests_import_api.${case}" COMMAND LVRSTests_import_api "${case}")
    set_tests_properties("LVRSTests_import_api.${case}" PROPERTIES
        ENVIRONMENT "${_import_api_test_environment}" TIMEOUT 60 SKIP_RETURN_CODE 77)
endforeach()
if(WIN32)
    set_property(TEST LVRSTests_import_api APPEND PROPERTY ENVIRONMENT "LVRS_IMPORT_CONTRACT_SET=native")
    add_test(NAME LVRSTests_import_toggle_pixel COMMAND LVRSTests_import_api toggle_switch_figma_contract_loads)
    set_tests_properties(LVRSTests_import_toggle_pixel PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=windows;QT_QUICK_CONTROLS_STYLE=Basic;QT_QUICK_BACKEND=rhi;QSG_RHI_BACKEND=d3d11;QML_IMPORT_PATH=${CMAKE_BINARY_DIR};QT_ENABLE_HIGHDPI_SCALING=0;QT_SCALE_FACTOR=1")
    add_test(NAME LVRSTests_window_chrome_offscreen COMMAND LVRSTests_import_api
        window_custom_chrome_interaction_contract_loads)
    set_tests_properties(LVRSTests_window_chrome_offscreen PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Basic;QT_QUICK_BACKEND=software;QML_IMPORT_PATH=${CMAKE_BINARY_DIR}")
endif()
uifw_add_test(LVRSTests_primary_color tst_primary_color.cpp)
get_test_property(LVRSTests_primary_color ENVIRONMENT _primary_color_test_environment)
foreach(case IN ITEMS
        binding_loop_warning_is_caught
        default_blue_is_compatible
        declarative_color:purple
        declarative_color:orange
        declarative_color:black
        declarative_color:alpha
        declarative_color:bootstrap
        declarative_color:shell
        initial_properties_color
        palette_can_be_observed_from_qml
        runtime_color_updates_controls_and_overlays
        secondary_windows_inherit_without_resetting
        engines_have_independent_themes
        custom_color_is_rendered
        catalog_accent_choices_update_the_app)
    add_test(NAME "LVRSTests_primary_color.${case}" COMMAND LVRSTests_primary_color "${case}")
    set_tests_properties("LVRSTests_primary_color.${case}" PROPERTIES
        ENVIRONMENT "${_primary_color_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_list_composites tst_list_composites.cpp)
get_test_property(LVRSTests_list_composites ENVIRONMENT _list_composites_test_environment)
set(_list_composites_cases mixed_list_uses_each_rows_height controls_deliver_real_edits_and_actions
    model_edits_keep_the_live_delegate long_content_and_custom_composition_fit
    variant_changes_reflow_and_keep_footer_fixed render_catalog_has_all_seventeen_rows
    visual_catalog_list_preview_loads)
foreach(variant IN ITEMS Mini Detail Navigation Toggle Checkable Action ActionGroup Stepper Select InlineEdit
        DetailActions DetailQuantity DetailSettings Resource Media Task Form)
    list(APPEND _list_composites_cases "figma_variants:${variant}" "figma_variants:${variant}-mobile")
endforeach()
foreach(case IN LISTS _list_composites_cases)
    add_test(NAME "LVRSTests_list_composites.${case}" COMMAND LVRSTests_list_composites "${case}")
    set_tests_properties("LVRSTests_list_composites.${case}" PROPERTIES
        ENVIRONMENT "${_list_composites_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_card tst_card.cpp)
get_test_property(LVRSTests_card ENVIRONMENT _card_test_environment)
set(_card_cases mouse_keyboard_and_nested_actions content_reflows_and_preview_swaps
    preview_errors_and_fill_pixels catalog_renders_all_designs)
foreach(mobile IN ITEMS 0 1)
    foreach(type RANGE 0 7)
        if(type EQUAL 0)
            set(_card_size_max 2)
            set(_card_detail_max 1)
        else()
            set(_card_size_max 0)
            set(_card_detail_max 0)
        endif()
        if(type EQUAL 1)
            set(_card_state_max 1)
        else()
            set(_card_state_max 3)
        endif()
        foreach(size RANGE 0 ${_card_size_max})
            foreach(detail RANGE 0 ${_card_detail_max})
                foreach(state RANGE 1 ${_card_state_max})
                    list(APPEND _card_cases "figma_contract:${type}-${size}-${detail}-${state}-${mobile}")
                endforeach()
            endforeach()
        endforeach()
    endforeach()
endforeach()
foreach(case IN LISTS _card_cases)
    add_test(NAME "LVRSTests_card.${case}" COMMAND LVRSTests_card "${case}")
    set_tests_properties("LVRSTests_card.${case}" PROPERTIES
        ENVIRONMENT "${_card_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_avatar tst_avatar.cpp)
uifw_add_test(LVRSTests_slider tst_slider.cpp)
get_test_property(LVRSTests_slider ENVIRONMENT _slider_test_environment)
set(_slider_cases ranges_resize_and_state_cycles keyboard_mouse_touch_and_snapping
    right_to_left_and_descending_ranges labels_icons_and_disabled catalog_renders_all_designs
    accent_pixel_matches_token)
foreach(platform IN ITEMS macos ios)
    foreach(type RANGE 0 6)
        foreach(size RANGE 0 3)
            foreach(state RANGE 1 4)
                list(APPEND _slider_cases "figma_variants:${platform}-${type}-${size}-${state}")
            endforeach()
        endforeach()
    endforeach()
endforeach()
foreach(case IN LISTS _slider_cases)
    add_test(NAME "LVRSTests_slider.${case}" COMMAND LVRSTests_slider "${case}")
    set_tests_properties("LVRSTests_slider.${case}" PROPERTIES
        ENVIRONMENT "${_slider_test_environment}" TIMEOUT 60)
endforeach()
add_executable(LVRSTests_ctest_touch_support ctest_touch_support.cpp)
target_link_libraries(LVRSTests_ctest_touch_support PRIVATE Qt6::Gui)
add_test(NAME LVRSTests_ctest_touch_support COMMAND LVRSTests_ctest_touch_support)
uifw_add_test(LVRSTests_tabs tst_tabs.cpp)
set_tests_properties(LVRSTests_tabs PROPERTIES SKIP_RETURN_CODE 77)
get_test_property(LVRSTests_tabs ENVIRONMENT _tabs_test_environment)
foreach(case IN ITEMS
        icon_is_required:Tab
        icon_is_required:MobileTab
        icon_is_required:MobileNavigationTab
        icon_is_always_left_of_label
        empty_icon_cannot_activate
        desktop_variants
        missing_model_icon_is_disabled
        desktop_icon_capture
        activation_focus_disabled_and_controlled_selection
        mobile_geometry_touch_and_presentations
        gallery_capture
    )
    add_test(NAME "LVRSTests_tabs.${case}" COMMAND LVRSTests_tabs "${case}")
    set_tests_properties("LVRSTests_tabs.${case}" PROPERTIES
        ENVIRONMENT "${_tabs_test_environment}" TIMEOUT 60 SKIP_RETURN_CODE 77)
endforeach()
uifw_add_test(LVRSTests_mobile_navigation tst_mobile_navigation.cpp)
set_tests_properties(LVRSTests_mobile_navigation PROPERTIES SKIP_RETURN_CODE 77)
get_test_property(LVRSTests_mobile_navigation ENVIRONMENT _mobile_navigation_test_environment)
foreach(case IN ITEMS
        selected_icons_use_darker_accent_token
        source_icon_colors_and_artwork_geometry
        gallery_capture
        left_alignment_search_radius_and_labels
        input_disabled_controlled_selection_and_overflow
        optional_search_reclaims_space_and_skips_focus
        shared_indicator_stretches_retargets_and_reduces_motion
    )
    add_test(NAME "LVRSTests_mobile_navigation.${case}" COMMAND LVRSTests_mobile_navigation "${case}")
    set_tests_properties("LVRSTests_mobile_navigation.${case}" PROPERTIES
        ENVIRONMENT "${_mobile_navigation_test_environment}" TIMEOUT 60 SKIP_RETURN_CODE 77)
endforeach()
uifw_add_test(LVRSTests_toggle_switch tst_toggle_switch.cpp)
get_test_property(LVRSTests_toggle_switch ENVIRONMENT _toggle_switch_test_environment)
foreach(case IN ITEMS
        press_and_rebound:off-to-on
        press_and_rebound:on-to-off
        quick_click_and_retarget
        keyboard_and_cancelled_press
        touch_press_and_rebound
        drag_and_mirroring
        immediate_and_disabled_states
    )
    add_test(NAME "LVRSTests_toggle_switch.${case}" COMMAND LVRSTests_toggle_switch "${case}")
    set_tests_properties("LVRSTests_toggle_switch.${case}" PROPERTIES
        ENVIRONMENT "${_toggle_switch_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_motion tst_motion.cpp)
get_test_property(LVRSTests_motion ENVIRONMENT _motion_test_environment)
set(_motion_cases button_press_rebound_and_retarget preferences_settle_in_flight_motion
    keyboard_disabled_and_layout slider_drag_is_direct_and_release_settles
    slider_programmatic_travel_stays_bounded touch_keeps_the_input_target_fixed hierarchy_mobile_release_and_cancel)
foreach(type IN ITEMS AbstractButton PushButton DropdownButton LabelButton IconButton LabelMenuButton IconMenuButton
        CheckBox RadioButton Card AlertButton Link ToolbarButton HierarchyItem ListItem MenuItem ContextMenuItem Stepper ComboBox)
    list(APPEND _motion_cases "button_families:${type}")
endforeach()
foreach(type IN ITEMS PushButton DropdownButton IconButton IconMenuButton MenuItem ContextMenuItem ListItem HierarchyItem)
    list(APPEND _motion_cases "compact_button_release:${type}-pointer" "compact_button_release:${type}-keyboard")
endforeach()
foreach(type IN ITEMS PushButton MenuItem ContextMenuItem ListItem HierarchyItem)
    list(APPEND _motion_cases "compact_button_cancel_and_opt_out:${type}")
endforeach()
foreach(type IN ITEMS PushButton DropdownButton IconButton IconMenuButton)
    foreach(tone RANGE 0 4)
        list(APPEND _motion_cases "compact_button_tones_and_keyboard_focus:${type}-${tone}")
    endforeach()
endforeach()
foreach(type IN ITEMS MenuItem ContextMenuItem HierarchyItem)
    foreach(state RANGE 0 2)
        list(APPEND _motion_cases "row_interaction_states:${type}-${state}")
    endforeach()
endforeach()
foreach(variant RANGE 0 16)
    list(APPEND _motion_cases "row_interaction_states:ListItem-${variant}")
endforeach()
list(APPEND _motion_cases "row_interaction_states:ListItem-selected" "row_interaction_states:ListItem-disabled")
foreach(type IN ITEMS PushButton DropdownButton MenuItem ContextMenuItem ListItem HierarchyItem)
    list(APPEND _motion_cases "enter_activates_on_release:${type}")
endforeach()
foreach(case IN LISTS _motion_cases)
    add_test(NAME "LVRSTests_motion.${case}" COMMAND LVRSTests_motion "${case}")
    set_tests_properties("LVRSTests_motion.${case}" PROPERTIES
        ENVIRONMENT "${_motion_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_instance_state tst_instance_state.cpp)
set_tests_properties(LVRSTests_instance_state PROPERTIES SKIP_RETURN_CODE 77)
get_test_property(LVRSTests_instance_state ENVIRONMENT _instance_state_test_environment)
set(_instance_state_cases cancellation_and_motion_preferences model_states_remain_independent
    mobile_hierarchy_input native_focus_capture recorded_figma_contract)
foreach(type IN ITEMS PushButton DropdownButton IconButton IconMenuButton MenuItem ContextMenuItem HierarchyItem)
    list(APPEND _instance_state_cases "owned_input_lifecycle:${type}")
endforeach()
foreach(type RANGE 0 16)
    list(APPEND _instance_state_cases "owned_input_lifecycle:ListItem-${type}")
endforeach()
foreach(state IN ITEMS collapsed expanded)
    foreach(color IN ITEMS "#0A84FF" "#A571E6")
        list(APPEND _instance_state_cases "menu_selected_uses_press_fill:${state}-${color}")
    endforeach()
endforeach()
foreach(case IN LISTS _instance_state_cases)
    add_test(NAME "LVRSTests_instance_state.${case}" COMMAND LVRSTests_instance_state "${case}")
    set_tests_properties("LVRSTests_instance_state.${case}" PROPERTIES
        ENVIRONMENT "${_instance_state_test_environment}" TIMEOUT 60 SKIP_RETURN_CODE 77)
endforeach()
uifw_add_test(LVRSTests_figma_parity tst_figma_parity.cpp)
set_tests_properties(LVRSTests_figma_parity PROPERTIES SKIP_RETURN_CODE 77)
get_test_property(LVRSTests_figma_parity ENVIRONMENT _figma_parity_test_environment)
foreach(case IN ITEMS tokens_and_icon_names saved_theme_tokens compatibility_accent_aliases_remain_fixed
        authored_accent_palette_is_rendered accent_role_tokens_match_figma icon_accent_tokens_match_figma
        material_background_tokens_match_figma legacy_theme_aliases saved_text_style_bindings
        saved_text_style_font_recipes component_geometry menu_density_and_typography
        button_states_and_activation footer_overrides material_inner_highlight text_layout_keeps_design_metrics)
    add_test(NAME "LVRSTests_figma_parity.${case}" COMMAND LVRSTests_figma_parity "${case}")
    set_tests_properties("LVRSTests_figma_parity.${case}" PROPERTIES
        ENVIRONMENT "${_figma_parity_test_environment}" TIMEOUT 60 SKIP_RETURN_CODE 77)
endforeach()
uifw_add_test(LVRSTests_colorpicker tst_colorpicker.cpp)
get_test_property(LVRSTests_colorpicker ENVIRONMENT _colorpicker_test_environment)
set(_colorpicker_cases authored_palette_defaults color_conversion_and_validation
    achromatic_hue_and_cmyk_channels external_binding_and_accept_cancel
    component_hosts_and_modal_lifecycle pointer_keyboard_touch_and_fields
    all_color_domains catalog_renders_six_views)
foreach(type RANGE 0 5)
    list(APPEND _colorpicker_cases "content_variants:${type}")
endforeach()
foreach(case IN LISTS _colorpicker_cases)
    add_test(NAME "LVRSTests_colorpicker.${case}" COMMAND LVRSTests_colorpicker "${case}")
    set_tests_properties("LVRSTests_colorpicker.${case}" PROPERTIES
        ENVIRONMENT "${_colorpicker_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_sheet tst_sheet.cpp)
get_test_property(LVRSTests_sheet ENVIRONMENT _sheet_test_environment)
foreach(case IN ITEMS
        placement_content_and_radius
        inline_content_and_bounded_scroll
        modal_input_dismissal_and_focus
        automatic_presentation_and_component_replacement
        animated_mobile_drag
        gallery_loads
    )
    add_test(NAME "LVRSTests_sheet.${case}" COMMAND LVRSTests_sheet "${case}")
    set_tests_properties("LVRSTests_sheet.${case}" PROPERTIES
        ENVIRONMENT "${_sheet_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_tooltip tst_tooltip.cpp)
get_test_property(LVRSTests_tooltip ENVIRONMENT _tooltip_test_environment)
set(_tooltip_cases constrained_viewport content_replacement_and_scrolling transformed_target_and_resize
    hover_delay_timeout_and_escape chrome_target_above_content_safe_area touch_hold_and_nonmodal_input gallery_loads)
foreach(placement IN ITEMS above below left right top-left top-right bottom-left bottom-right flip-preference)
    list(APPEND _tooltip_cases "placement_and_tail:${placement}")
endforeach()
foreach(hover IN ITEMS controls-body controls-chrome lvrs-body lvrs-chrome)
    list(APPEND _tooltip_cases "automatic_hover_does_not_consume_clicks:${hover}")
endforeach()
foreach(case IN LISTS _tooltip_cases)
    add_test(NAME "LVRSTests_tooltip.${case}" COMMAND LVRSTests_tooltip "${case}")
    set_tests_properties("LVRSTests_tooltip.${case}" PROPERTIES
        ENVIRONMENT "${_tooltip_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_runtimeevents tst_runtimeevents.cpp)
uifw_add_test(LVRSTests_backend_io tst_backend_io.cpp)
get_test_property(LVRSTests_backend_io ENVIRONMENT _backend_io_test_environment)
foreach(case IN ITEMS
        backend_file_roundtrip_and_errors
        backend_writable_location_contract_respects_platform_defaults
        backend_error_signal_and_directory_idempotence
        backend_event_hook_receives_runtime_events
        backend_async_concurrency_and_io_contract
        backend_async_delay_distribution_and_read_cache_contract
        backend_performance_metrics_and_trace_contract
        backend_p1_scheduler_backpressure_cancel_and_cache_capacity_contract
    )
    add_test(NAME "LVRSTests_backend_io.${case}" COMMAND LVRSTests_backend_io "${case}")
    set_tests_properties("LVRSTests_backend_io.${case}" PROPERTIES
        ENVIRONMENT "${_backend_io_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_app_state tst_app_state.cpp)
uifw_add_test(LVRSTests_render_quality tst_render_quality.cpp)
get_test_property(LVRSTests_render_quality ENVIRONMENT _render_quality_test_environment)
foreach(case IN ITEMS
        render_quality_bounds_and_window_apply
        render_quality_signal_and_global_defaults
        render_quality_gpu_policy_pso_texture_and_drs_contract)
    add_test(NAME "LVRSTests_render_quality.${case}" COMMAND LVRSTests_render_quality "${case}")
    set_tests_properties("LVRSTests_render_quality.${case}" PROPERTIES
        ENVIRONMENT "${_render_quality_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_font_policy tst_font_policy.cpp)
uifw_add_test(LVRSTests_navigation_state tst_navigation_state.cpp)
get_test_property(LVRSTests_navigation_state ENVIRONMENT _navigation_state_test_environment)
foreach(case IN ITEMS
        page_monitor_history_metrics page_monitor_signal_contract_and_normalization
        view_state_tracker_syncs_stack_and_status view_state_tracker_disable_override_changes_active_target
        route_matcher_normalization_and_param_contract viewmodels_registry_binding_ownership_and_write_permissions
        viewmodels_registry_rebinding_clears_stale_ownership viewmodels_registry_tracks_keys_and_ownership
        viewmodels_registry_signal_and_prune_contract viewmodel_base_and_registry_descriptors_track_cpp_state
        state_model_stores_state_and_registry_descriptors_track_values
        cpp_model_sources_own_list_hierarchy_and_table_contracts table_projection_matches_public_cell_queries
        table_viewport_projection_preserves_merged_anchors_and_bounds table_column_count_tracks_headers_structure_and_undo
        qml_context_bind_plan_exposes_context_objects_and_viewmodels qml_context_bind_plan_reports_missing_required_objects)
    add_test(NAME "LVRSTests_navigation_state.${case}" COMMAND LVRSTests_navigation_state "${case}")
    set_tests_properties("LVRSTests_navigation_state.${case}" PROPERTIES
        ENVIRONMENT "${_navigation_state_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_qml_type_registrar tst_qml_type_registrar.cpp)
uifw_add_test(LVRSTests_runtime_services tst_runtime_services.cpp)
get_test_property(LVRSTests_runtime_services ENVIRONMENT _runtime_services_test_environment)
foreach(case IN ITEMS
        runtime_events_measurement_boundaries runtime_events_daemon_health_and_recent_log
        runtime_events_idle_and_reset_signal_contract runtime_events_context_menu_signal_contract
        runtime_events_single_input_event_is_counted_once runtime_events_touch_input_normalizes_to_pointer_contract
        runtime_events_touch_payload_exposes_native_point_details runtime_events_hit_test_exposes_ui_hierarchy_metadata
        gesture_events_recognize_hold_drag_and_swipe gesture_events_classify_mobile_press_scroll_and_multitouch
        runtime_events_window_replacement_survives_old_window_destruction render_monitor_counts_frames_when_swapped
        render_monitor_active_signal_and_destroy_path debug_logger_enabled_signal_and_message_format)
    add_test(NAME "LVRSTests_runtime_services.${case}" COMMAND LVRSTests_runtime_services "${case}")
    set_tests_properties("LVRSTests_runtime_services.${case}" PROPERTIES
        ENVIRONMENT "${_runtime_services_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_svg_manager tst_svg_manager.cpp)
# Embed exact, unnormalized source bytes once instead of reopening thousands of
# external-storage files during each geometry/raster check.
file(GLOB_RECURSE _lvrs_test_svg_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../resources/iconset/*.svg"
    "${CMAKE_CURRENT_SOURCE_DIR}/../resources/images/*.svg")
qt_add_resources(LVRSTests_svg_manager lvrs_test_source_svg
    PREFIX "/lvrs-test-source-svg" BASE "${CMAKE_CURRENT_SOURCE_DIR}/../resources"
    FILES ${_lvrs_test_svg_sources})
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_executable(LVRSTests_ctest_support ctest_support.cpp)
add_test(NAME LVRSTests_ctest_support COMMAND LVRSTests_ctest_support)
add_test(NAME LVRSTests_core_ctest_contract
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/test_ctest_core_contract.py")
add_test(NAME LVRSTests_figma_color_tokens
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/test_figma_color_tokens.py")
add_test(NAME LVRSTests_figma_iconset_import
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/test_figma_iconset_import.py")
add_test(NAME LVRSTests_accent_palette
    COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/test_accent_palette.py")
add_executable(LVRSNormalizeSvgIcons EXCLUDE_FROM_ALL ../scripts/normalize_svg_icons.cpp)
target_compile_features(LVRSNormalizeSvgIcons PRIVATE cxx_std_23)
target_link_libraries(LVRSNormalizeSvgIcons PRIVATE Qt6::Core Qt6::Gui Qt6::Svg)
uifw_add_test(LVRSTests_platform_integration tst_platform_integration.cpp)
set_tests_properties(LVRSTests_platform_integration PROPERTIES SKIP_RETURN_CODE 77)
get_test_property(LVRSTests_platform_integration ENVIRONMENT _platform_integration_test_environment)
foreach(case IN ITEMS
        safe_area_tracks_window_lifetime
        platform_flags_consistency
        platform_runtime_profiles_are_exposed
        application_window_and_main_metrics_are_exposed
        mobile_theme_scale_contract
        native_window_interaction_resizes_manually_when_system_resize_is_unavailable
        native_window_interaction_validates_system_resize_edges
        native_window_style_mobile_coverage_flags)
    add_test(NAME "LVRSTests_platform_integration.${case}" COMMAND LVRSTests_platform_integration "${case}")
    set_tests_properties("LVRSTests_platform_integration.${case}" PROPERTIES
        ENVIRONMENT "${_platform_integration_test_environment}" TIMEOUT 60 SKIP_RETURN_CODE 77)
endforeach()
uifw_add_test(LVRSTests_page_router tst_page_router.cpp)
uifw_add_test(LVRSTests_code_editor tst_code_editor.cpp)
get_test_property(LVRSTests_code_editor ENVIRONMENT _code_editor_test_environment)
foreach(case IN ITEMS
        code_editor_default_contract_and_utility_api
        code_editor_submit_signal_and_fixed_plain_text_mode
        code_editor_ios_native_text_interaction_contract
        code_editor_native_event_input_matrix
        code_editor_ios_scroll_physics_contract)
    add_test(NAME "LVRSTests_code_editor.${case}" COMMAND LVRSTests_code_editor "${case}")
    set_tests_properties("LVRSTests_code_editor.${case}" PROPERTIES
        ENVIRONMENT "${_code_editor_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_text_editor tst_text_editor.cpp)
get_test_property(LVRSTests_text_editor ENVIRONMENT _text_editor_test_environment)
foreach(case IN ITEMS
        text_editor_requires_file_path_contract
        text_editor_rich_text_surface_contract
        text_editor_document_edited_emits_latest_payload_and_revision
        text_editor_mobile_native_gesture_viewport_scroll_contract
        text_editor_api_usage_manual_contract
        text_editor_rich_text_file_realtime_sync_contract
        text_editor_return_key_moves_caret_to_inserted_line)
    add_test(NAME "LVRSTests_text_editor.${case}" COMMAND LVRSTests_text_editor "${case}")
    set_tests_properties("LVRSTests_text_editor.${case}" PROPERTIES
        ENVIRONMENT "${_text_editor_test_environment}" TIMEOUT 60)
endforeach()
uifw_add_test(LVRSTests_text_markup tst_text_markup.cpp)
uifw_add_test(LVRSTests_hierarchy_codec tst_hierarchy_codec.cpp)
uifw_add_test(LVRSTests_hierarchy_interaction tst_hierarchy_interaction.cpp)
uifw_add_test(LVRSTests_text_document_model tst_text_document_model.cpp)
uifw_add_test(LVRSTests_performance_gate tst_performance_gate.cpp LABELS "p4;quality;performance;ci")
uifw_add_test(LVRSTests_visual_regression tst_visual_regression.cpp LABELS "p4;quality;visual;ci")
  # The shared pixel golden is a raster reference. GPU backends use different
  # rounded-edge tessellation; component/effect tests still run on native RHI.
  set_property(TEST LVRSTests_visual_regression APPEND PROPERTY ENVIRONMENT "QT_QUICK_BACKEND=software")
if(WIN32)
    foreach(_pixel_test IN ITEMS LVRSTests_visual_regression LVRSTests_list_composites
            LVRSTests_slider LVRSTests_instance_state)
        set_property(TEST ${_pixel_test} APPEND PROPERTY ENVIRONMENT
            "QT_ENABLE_HIGHDPI_SCALING=0" "QT_SCALE_FACTOR=1")
    endforeach()
endif()
uifw_add_test(LVRSTests_soak_runtime tst_soak_runtime.cpp LABELS "p4;quality;soak;long")
if(WIN32)
    target_link_libraries(LVRSTests_performance_gate PRIVATE winmm)
    target_link_libraries(LVRSTests_soak_runtime PRIVATE winmm)
endif()

add_test(
    NAME LVRSTests_cmake_install_defaults
    COMMAND "${CMAKE_COMMAND}"
        "-DLVRS_SOURCE_DIR=${CMAKE_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_install_defaults.cmake"
)
add_test(NAME LVRSTests_bootstrap_shared_policy COMMAND "${CMAKE_COMMAND}"
    "-DACTION=${CMAKE_SOURCE_DIR}/cmake/LVRSBootstrapFrameworkAction.cmake"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_bootstrap_shared_policy.cmake")
