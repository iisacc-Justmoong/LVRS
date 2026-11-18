from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
CASES = {'hierarchy_codec': ('HierarchyCodecTests', ['parse_nodes_schema_succeeds', 'reparent_prevents_cycle']), 'text_markup': ('TextMarkupTests', ['normalize_converts_html_markup_to_canonical_text', 'render_html_is_equivalent_for_markdown_and_html_inputs', 'render_plain_text_returns_normalized_text']), 'debug': ('DebugTests', ['debug_enabled_toggle']), 'windows_exports': ('', ['exportedModelsAndBootstrapLink']), 'examples': ('ExampleSmokeTests', ['ios_hello_example_loads', 'android_hello_example_loads', 'mvvm_example_loads', 'event_listener_example_loads', 'typing_practice_example_loads', 'visual_catalog_example_loads']), 'permission_sequencer': ('PermissionSequencerTests', ['sequencer_runs_steps_in_priority_order_and_records_history', 'sequencer_stops_on_required_failure_and_can_continue_when_configured'])}
CASES.update({
    'card': ('CardTests', [
        'mouse_keyboard_and_nested_actions', 'content_reflows_and_preview_swaps',
        'preview_errors_and_fill_pixels', 'catalog_renders_all_designs']),
    'slider': ('SliderTests', [
        'ranges_resize_and_state_cycles', 'keyboard_mouse_touch_and_snapping',
        'right_to_left_and_descending_ranges', 'labels_icons_and_disabled',
        'catalog_renders_all_designs', 'accent_pixel_matches_token']),
    'platform_integration': ('PlatformIntegrationTests', [
        'safe_area_tracks_window_lifetime', 'platform_flags_consistency',
        'platform_runtime_profiles_are_exposed', 'application_window_and_main_metrics_are_exposed',
        'mobile_theme_scale_contract',
        'native_window_interaction_resizes_manually_when_system_resize_is_unavailable',
        'native_window_interaction_validates_system_resize_edges',
        'native_window_style_mobile_coverage_flags']),
    'colorpicker': ('ColorPickerTests', [
        'authored_palette_defaults', 'color_conversion_and_validation',
        'achromatic_hue_and_cmyk_channels', 'external_binding_and_accept_cancel',
        'component_hosts_and_modal_lifecycle', 'pointer_keyboard_touch_and_fields',
        'all_color_domains', 'catalog_renders_six_views']),
    'list_composites': ('ListCompositeTests', [
        'mixed_list_uses_each_rows_height', 'controls_deliver_real_edits_and_actions',
        'model_edits_keep_the_live_delegate', 'long_content_and_custom_composition_fit',
        'variant_changes_reflow_and_keep_footer_fixed', 'render_catalog_has_all_seventeen_rows',
        'visual_catalog_list_preview_loads']),
    'instance_state': ('InstanceStateTests', [
        'cancellation_and_motion_preferences', 'model_states_remain_independent',
        'mobile_hierarchy_input', 'native_focus_capture', 'recorded_figma_contract']),
    'tooltip': ('TooltipTests', [
        'constrained_viewport', 'content_replacement_and_scrolling',
        'transformed_target_and_resize', 'hover_delay_timeout_and_escape',
        'chrome_target_above_content_safe_area', 'touch_hold_and_nonmodal_input', 'gallery_loads']),
    'navigation_state': ('NavigationStateTests', [
        'page_monitor_history_metrics', 'page_monitor_signal_contract_and_normalization',
        'view_state_tracker_syncs_stack_and_status', 'view_state_tracker_disable_override_changes_active_target',
        'route_matcher_normalization_and_param_contract', 'viewmodels_registry_binding_ownership_and_write_permissions',
        'viewmodels_registry_rebinding_clears_stale_ownership', 'viewmodels_registry_tracks_keys_and_ownership',
        'viewmodels_registry_signal_and_prune_contract', 'viewmodel_base_and_registry_descriptors_track_cpp_state',
        'state_model_stores_state_and_registry_descriptors_track_values',
        'cpp_model_sources_own_list_hierarchy_and_table_contracts', 'table_projection_matches_public_cell_queries',
        'table_viewport_projection_preserves_merged_anchors_and_bounds',
        'table_column_count_tracks_headers_structure_and_undo',
        'qml_context_bind_plan_exposes_context_objects_and_viewmodels',
        'qml_context_bind_plan_reports_missing_required_objects']),
    'runtime_services': ('RuntimeServicesTests', [
        'runtime_events_measurement_boundaries', 'runtime_events_daemon_health_and_recent_log',
        'runtime_events_idle_and_reset_signal_contract', 'runtime_events_context_menu_signal_contract',
        'runtime_events_single_input_event_is_counted_once', 'runtime_events_touch_input_normalizes_to_pointer_contract',
        'runtime_events_touch_payload_exposes_native_point_details', 'runtime_events_hit_test_exposes_ui_hierarchy_metadata',
        'gesture_events_recognize_hold_drag_and_swipe', 'gesture_events_classify_mobile_press_scroll_and_multitouch',
        'runtime_events_window_replacement_survives_old_window_destruction', 'render_monitor_counts_frames_when_swapped',
        'render_monitor_active_signal_and_destroy_path', 'debug_logger_enabled_signal_and_message_format']),
    'figma_parity': ('FigmaParityTests', [
        'tokens_and_icon_names', 'saved_theme_tokens', 'compatibility_accent_aliases_remain_fixed',
        'authored_accent_palette_is_rendered', 'accent_role_tokens_match_figma',
        'icon_accent_tokens_match_figma', 'material_background_tokens_match_figma',
        'legacy_theme_aliases', 'saved_text_style_bindings', 'saved_text_style_font_recipes',
        'component_geometry', 'menu_density_and_typography', 'button_states_and_activation',
        'footer_overrides', 'material_inner_highlight', 'text_layout_keeps_design_metrics']),
    'motion': ('MotionTests', [
        'button_press_rebound_and_retarget', 'preferences_settle_in_flight_motion',
        'keyboard_disabled_and_layout', 'slider_drag_is_direct_and_release_settles',
        'slider_programmatic_travel_stays_bounded', 'touch_keeps_the_input_target_fixed',
        'hierarchy_mobile_release_and_cancel']),
    'toggle_switch': ('ToggleSwitchTests', [
        'press_and_rebound:off-to-on', 'press_and_rebound:on-to-off',
        'quick_click_and_retarget', 'keyboard_and_cancelled_press',
        'touch_press_and_rebound', 'drag_and_mirroring', 'immediate_and_disabled_states']),
    'mobile_navigation': ('MobileNavigationTests', [
        'selected_icons_use_darker_accent_token', 'source_icon_colors_and_artwork_geometry',
        'gallery_capture', 'left_alignment_search_radius_and_labels',
        'input_disabled_controlled_selection_and_overflow',
        'optional_search_reclaims_space_and_skips_focus',
        'shared_indicator_stretches_retargets_and_reduces_motion']),
    'tabs': ('TabsTests', [
        'icon_is_required:Tab', 'icon_is_required:MobileTab',
        'icon_is_required:MobileNavigationTab', 'icon_is_always_left_of_label',
        'empty_icon_cannot_activate', 'desktop_variants', 'missing_model_icon_is_disabled',
        'desktop_icon_capture', 'activation_focus_disabled_and_controlled_selection',
        'mobile_geometry_touch_and_presentations', 'gallery_capture']),
    'materials': ('MaterialTests', [
        'window_default_and_primary', 'window_material_inherits_owning_window_accent',
        'application_window_theme_accent_mix', 'application_window_custom_background',
        'noise_texture_defaults_match_figma', 'noise_texture_inputs_are_bounded_and_opaque',
        'window_gradient_tracks_accent_and_resize', 'transient_defaults_and_capture_safety',
        'backdrop_blur_and_color_rendering', 'catalog_transient_interaction',
        'context_menu_frost_contract', 'context_menu_frost_rendering']),
    'sheet': ('SheetTests', [
        'placement_content_and_radius', 'inline_content_and_bounded_scroll',
        'modal_input_dismissal_and_focus', 'automatic_presentation_and_component_replacement',
        'animated_mobile_drag', 'gallery_loads']),
    'backend_io': ('BackendIoTests', [
        'backend_file_roundtrip_and_errors',
        'backend_writable_location_contract_respects_platform_defaults',
        'backend_error_signal_and_directory_idempotence',
        'backend_event_hook_receives_runtime_events',
        'backend_async_concurrency_and_io_contract',
        'backend_async_delay_distribution_and_read_cache_contract',
        'backend_performance_metrics_and_trace_contract',
        'backend_p1_scheduler_backpressure_cancel_and_cache_capacity_contract']),
    'primary_color': ('PrimaryColorTests', [
        'default_blue_is_compatible', 'declarative_color:purple',
        'declarative_color:orange', 'declarative_color:black',
        'declarative_color:alpha', 'declarative_color:bootstrap',
        'declarative_color:shell', 'initial_properties_color',
        'palette_can_be_observed_from_qml', 'runtime_color_updates_controls_and_overlays',
        'secondary_windows_inherit_without_resetting', 'engines_have_independent_themes',
        'custom_color_is_rendered', 'catalog_accent_choices_update_the_app',
        'binding_loop_warning_is_caught']),
    'render_quality': ('RenderQualityTests', [
        'render_quality_bounds_and_window_apply',
        'render_quality_signal_and_global_defaults',
        'render_quality_gpu_policy_pso_texture_and_drs_contract']),
    'text_editor': ('TextEditorTests', [
        'text_editor_requires_file_path_contract',
        'text_editor_rich_text_surface_contract',
        'text_editor_document_edited_emits_latest_payload_and_revision',
        'text_editor_mobile_native_gesture_viewport_scroll_contract',
        'text_editor_api_usage_manual_contract',
        'text_editor_rich_text_file_realtime_sync_contract',
        'text_editor_return_key_moves_caret_to_inserted_line']),
    'code_editor': ('CodeEditorTests', [
        'code_editor_default_contract_and_utility_api',
        'code_editor_submit_signal_and_fixed_plain_text_mode',
        'code_editor_ios_native_text_interaction_contract',
        'code_editor_native_event_input_matrix',
        'code_editor_ios_scroll_physics_contract']),
    'text_document_model': ('TextDocumentStorageTests', [
        'provider_chunked_load_and_cancellation',
        'range_edits_preserve_unaffected_rows_and_counts',
        'saving_preserves_rows_indexes_and_text',
        'saving_rejects_missing_or_truncated_source']),
    'app_state': ('AppStateTests', ['defaults_and_progress_clamping', 'route_tracking_updates_models',
                                  'snapshot_sync_bridges_backend_state', 'resilient_normalization_and_index_clamping',
                                  'definitions_drive_route_matching']),
    'bootstrap_parallel': ('BootstrapParallelTests', ['parallel_tasks_run_concurrently_and_apply_on_receiver_thread',
                                                     'parallel_runner_reports_load_and_apply_failures']),
    'foreground_services': ('ForegroundServicesTests', ['gate_requires_visible_workspace_and_starts_services_once',
                                                       'gate_reports_failures_and_can_be_reset']),
    'font_policy': ('FontPolicyTests', ['font_policy_token_mapping_is_strict',
                                       'font_policy_family_resolution_and_compliance_edges']),
    'hierarchy_interaction': ('HierarchyInteractionTests', ['move_intent_before_after_child_root',
        'visibility_projection_respects_expanded_state', 'visibility_projection_matches_reference',
        'visibility_projection_handles_deep_and_cyclic_branches', 'undo_redo_roundtrip']),
    'layout': ('LayoutTests', ['vstack_default_spacing', 'hstack_default_spacing',
        'vstack_alignment_name', 'hstack_alignment_name', 'spacer_min_length_vertical']),
    'runtimeevents': ('RuntimeEventsTests', ['runtime_events_are_exposed_and_idle_transitions',
        'keyboard_and_mouse_events_are_captured', 'daemon_contract_is_exposed_to_qml']),
    'qml_type_registrar': ('QmlTypeRegistrarTests', ['manifest_registers_types_and_qml_can_create_manifest_type',
        'manifest_reports_required_failures_optional_skips_and_duplicates']),
    'performance_gate': ('PerformanceGateTests', ['backend_dispatch_latency_regression_gate']),
    'catalog': ('CatalogTests', ['every_qml_type_has_a_recipe_and_live_preview', 'search_and_reset', 'native_catalog_capture']),
    'visual_regression': ('VisualRegressionTests', ['comparison_detects_changes', 'reference_scene_matches_golden']),
    'windows_qml_link': ('', []),
    'svg_manager': ('SvgManagerTests', ['svg_manager_defaults_to_square_18_logical_pixels',
        'svg_manager_generates_png_and_clamps', 'svg_manager_error_paths_and_cache_signals',
        'figma_iconset_resources_are_complete_and_renderable', 'control_variant_resources_are_packaged',
        'provider_metadata_is_not_packaged', 'translate_object_preserves_image_alpha',
        'smart_select_preserves_image_alpha', 'all_svg_icons_scale_and_center']),
    'soak_runtime': ('SoakRuntimeTests', ['backend_io_dispatch_soak_contract']),
    'app_entry': ('AppEntryTests', ['default_runtime_qml_import_paths_include_staged_bundle_dirs',
        'default_runtime_qml_import_paths_include_snapshot_platform_dirs', 'bootstrap_diagnostics_are_emitted',
        'qml_root_loader_loads_multiple_roots_with_initial_properties', 'qml_root_loader_reports_missing_root_object',
        'qml_root_loader_applies_window_activation_policy', 'lifecycle_stage_runs_hook_and_priority_tasks',
        'lifecycle_queue_reports_task_failures_and_fatality', 'lifecycle_stage_can_be_scheduled_for_first_idle']),
    'eventlistener': ('EventListenerTests', ['click_trigger', 'global_context_requested_trigger',
        'application_window_global_context_signal', 'context_menu_dismisses_on_global_press_outside',
        'gesture_triggers_receive_touch_and_swipe', 'mobile_press_and_scroll_triggers_are_distinct']),
    'page_router': ('PageRouterTests', ['route_params_are_passed_to_target_component',
        'default_initial_path_does_not_override_bound_path', 'component_navigation_keeps_path_stack_in_sync',
        'route_pages_are_isolated_and_forced_to_viewport', 'page_router_retain_inactive_depth_contract',
        'page_router_updates_view_state_tracker_from_stack', 'global_navigator_allows_one_line_navigation',
        'global_navigator_falls_back_to_previous_router', 'interactive_navigation_transitions_hold_route_commit_until_finish',
        'page_transition_controller_proxies_application_window_router', 'route_mvvm_binding_and_write_ownership_are_applied',
        'route_mvvm_ownership_is_released_when_view_is_popped']),
})

CASES['import_api'] = ('ImportApiTests', [
    'embedded_qml_matches_source',
    'app_bootstrap_window_loads',
    'versionless_import_application_window_loads',
    'application_window_mobile_coverage_visibility_contract_loads',
    'application_window_page_stack_state_loads',
    'application_window_initial_properties_seed_page_stack',
    'application_window_safe_margin_scopes_to_layout_not_render_surface',
    'versionless_import_window_loads',
    'window_custom_chrome_interaction_contract_loads',
    'window_safe_margin_scopes_to_layout_not_render_surface',
    'appshell_compat_loads',
    'application_window_platform_adaptive_layout_loads',
    'icon_name_mapping_loads',
    'compact_icon_frame_contract_loads',
    'hierarchy_tree_model_api_loads',
    'hierarchy_string_array_model_loads',
    'hierarchy_direct_model_contract_loads',
    'hierarchy_direct_model_editing_contract_loads',
    'hierarchy_nested_children_indent_contract_loads',
    'hierarchy_expansion_uses_incremental_visibility_refresh',
    'hierarchy_editable_drag_depth_contract_loads',
    'hierarchy_editable_drag_per_item_lock_contract_loads',
    'hierarchy_mobile_drag_hold_contract_loads',
    'hierarchy_mobile_activation_commits_on_release_contract_loads',
    'hierarchy_mobile_reactivation_reemits_active_signal',
    'hierarchy_mobile_scroll_physics_contract_loads',
    'hierarchy_optional_footer_contract_loads',
    'hierarchy_toolbar_empty_state_does_not_reschedule',
    'hierarchy_toolbar_item_model_contract_loads',
    'hierarchy_toolbar_figma_layout_contract_loads',
    'hierarchy_figma_composition_contract_loads',
    'hierarchy_toolbar_manual_icon_button_contract_loads',
    'hierarchy_row_click_only_activates_not_toggles',
    'hierarchy_chevron_requires_children_loads',
    'hierarchy_item_chevron_direction_contract_loads',
    'hierarchy_item_hover_and_active_state_visual_contract_loads',
    'hierarchy_item_figma_defaults_contract_loads',
    'hierarchy_item_layout_geometry_contract_loads',
    'hierarchy_item_count_view_contract_loads',
    'hierarchy_item_structure_api_contract_loads',
    'hierarchy_item_ux_state_contract_loads',
    'button_padding_matches_figma_spec',
    'button_default_tone_matches_figma_accent_loads',
    'segmented_button_selected_color',
    'button_injected_methods_contract_loads',
    'stepper_figma_contract_loads',
    'combo_box_figma_contract_loads',
    'input_field_figma_contract_loads',
    'input_field_material_rendering_contract',
    'progress_bar_range_contract_loads',
    'control_icons_use_supersampled_raster_contract',
    'input_field_ios_native_text_interaction_contract_loads',
    'input_field_native_event_input_contract_loads',
    'input_field_search_icon_mobile_scaling_contract_loads',
    'toggle_switch_figma_contract_loads',
    'checkbox_figma_contract_loads',
    'radio_button_figma_contract_loads',
    'modal_empty_frame_contract_loads',
    'modal_content_action_contract_loads',
    'alert_figma_variant_contract_loads',
    'alert_content_arguments_contract_loads',
    'alert_action_button_padding_scopes_to_alert',
    'menu_item_key_and_chevron_contract_loads',
    'menu_item_icon_slot_switch_contract_loads',
    'context_menu_item_action_contract_loads',
    'context_menu_visual_contract_loads',
    'context_menu_icon_slot_switch_contract_loads',
    'context_menu_width_expansion_contract_loads',
    'context_menu_auto_placement_contract_loads',
    'model_component_delegate_contract_loads',
    'table_figma_geometry_contract_loads',
    'table_spreadsheet_api_contract_loads',
    'table_viewport_reuses_delegates_and_creates_one_menu_on_demand',
    'table_cell_item_contract_loads',
    'table_cell_merge_split_contract_loads',
    'table_structure_editing_contract_loads',
    'table_resize_contract_loads',
    'table_typed_header_contract_loads',
    'table_cpp_model_contract_loads',
    'table_undo_redo_contract_loads',
    'list_model_contract_loads',
    'list_figma_contract_loads',
    'button_family_components_contract:desktop',
    'button_family_components_contract:mobile',
    'segmented_control_figma_contract_loads:desktop',
    'segmented_control_figma_contract_loads:mobile',
    'alert_button_methods_are_invoked:single-primary',
    'alert_button_methods_are_invoked:horizontal-primary',
    'alert_button_methods_are_invoked:horizontal-secondary',
    'alert_button_methods_are_invoked:vertical-primary',
    'alert_button_methods_are_invoked:vertical-secondary',
    'alert_button_methods_are_invoked:vertical-tertiary',
    'alert_glass_overlay_and_input_contract:qt-window',
    'alert_glass_overlay_and_input_contract:lvrs-window',
])

class CoreCTestContract(unittest.TestCase):
    def test_import_api_keeps_full_case_list_and_signal_observations(self):
        source = (ROOT / 'tests/tst_import_api.cpp').read_text()
        self.assertEqual(len(CASES['import_api'][1]), 94)
        for token in ('ImportSignalRecorder', 'recordVoid', 'recordActivation',
                      'waitForInputMaterials', 'SIGNAL(painted())', 'QQuickWindow::frameSwapped',
                      'activationSpyConnection', 'primarySpyConnection', 'dismissSpyConnection',
                      'LVRS_IMPORT_CONTRACT_SET', 'dataTag', 'QEvent::MouseButtonDblClick',
                      'QEvent::MouseButtonPress', 'QEvent::MouseButtonRelease',
                      'QEvent::KeyPress', 'QEvent::KeyRelease'):
            self.assertTrue(token in source, f'missing {token}')
        self.assertTrue('LVRSTests_import_api.${case}' in (ROOT / 'tests/CMakeLists.txt').read_text())

    def test_motion_preserves_data_signals_and_input_timing(self):
        source = (ROOT / 'tests/tst_motion.cpp').read_text()
        for token in ('MotionSignalRecorder', 'recordSignal', 'clicksConnection', 'cancelledConnection',
                      'button_families:', 'compact_button_release:', 'compact_button_cancel_and_opt_out:',
                      'compact_button_tones_and_keyboard_focus:', 'row_interaction_states:',
                      'enter_activates_on_release:', 'TouchSequence', 'Qt::ShiftModifier',
                      'activatable: false',
                      'QEvent::MouseButtonPress', 'QEvent::MouseButtonRelease', 'QEvent::MouseMove',
                      'QEvent::KeyPress', 'QEvent::KeyRelease', 'processFor(110)', 'processFor(220)'):
            self.assertTrue(token in source, f'missing {token}')
        self.assertTrue('LVRSTests_motion.${case}' in (ROOT / 'tests/CMakeLists.txt').read_text())

    def test_figma_parity_retains_fixture_rows_and_signal_values(self):
        source = (ROOT / 'tests/tst_figma_parity.cpp').read_text()
        for token in ('run_saved_theme_tokens_rows', 'run_legacy_theme_aliases_rows',
                      'run_saved_text_style_bindings_rows', 'run_saved_text_style_font_recipes_rows',
                      'run_component_geometry_rows', 'FigmaSignalRecorder', 'recordTriggered',
                      'recordClicked', 'triggeredEvents', 'figma-theme-contract.json',
                      'figma-contract.json', 'lvrs_ctest::skip(', 'QEvent::MouseButtonPress',
                      'QEvent::MouseButtonRelease', 'QEvent::KeyPress', 'QEvent::KeyRelease'):
            self.assertTrue(token in source, f'missing {token}')
        self.assertTrue('LVRSTests_figma_parity.${case}' in (ROOT / 'tests/CMakeLists.txt').read_text())

    def test_runtime_services_keep_event_payloads_and_timing(self):
        source = (ROOT / 'tests/tst_runtime_services.cpp').read_text()
        for token in ('contextSpyEvents', 'mousePressedSpyEvents', 'mouseMovedSpyEvents',
                      'mouseReleasedSpyEvents', 'touchStartedSpyEvents', 'touchEndedSpyEvents',
                      'pressStartedSpyEvents', 'scrollUpdatedSpyEvents', 'swipeDetectedSpyEvents',
                      'const QVariantMap &eventData', 'contextSpyEvents.takeLast()',
                      'touchStartedSpyEvents.constFirst()', 'scrollEndedSpyEvents.constFirst()',
                      'QObject::disconnect(', 'QTouchEvent', '400)'):
            self.assertTrue(token in source, f'missing {token}')
        self.assertTrue('LVRSTests_runtime_services.${case}' in (ROOT / 'tests/CMakeLists.txt').read_text())

    def test_navigation_state_records_signals_and_value_payloads(self):
        source = (ROOT / 'tests/tst_navigation_state.cpp').read_text()
        for token in ('historySpyEvents', 'stackSpyEvents', 'viewsSpyEvents',
                      'ownershipSpyEvents', 'errorSpyEvents', 'keysSpyEvents',
                      'descriptorSpyEvents', 'valuesSpyEvents', 'revisionSpyEvents',
                      'valueSpyEvents', 'key, value, previousValue',
                      'QObject::disconnect(', 'QAbstractListModel', 'ViewModelRegistry'):
            self.assertTrue(token in source, f'missing {token}')
        self.assertTrue('LVRSTests_navigation_state.${case}' in (ROOT / 'tests/CMakeLists.txt').read_text())

    def test_tooltip_keeps_placement_hover_capture_and_warning_recording(self):
        source = (ROOT / 'tests/tst_tooltip.cpp').read_text()
        for token in ('placement_and_tail:', 'automatic_hover_does_not_consume_clicks:',
                      'top-left', 'bottom-right', 'controls-chrome', 'lvrs-chrome',
                      'QQuickItemGrabResult::ready', 'captureConnection',
                      'QObject::disconnect(captureConnection)', 'bindingLoopWarnings',
                      'QQmlEngine::warnings', 'TouchSequence', 'LVRS_TEST_SOURCE_DIR',
                      'QEvent::MouseButtonPress', 'QEvent::MouseButtonRelease',
                      'QEvent::MouseMove', 'QEvent::KeyPress', 'QEvent::KeyRelease',
                      'LVRS_TOOLTIP_CAPTURE_DIR', 'processFor(600)'):
            self.assertTrue(token in source, f'missing {token}')
        self.assertFalse('QFINDTESTDATA' in source)
        self.assertTrue('LVRSTests_tooltip.${case}' in (ROOT / 'tests/CMakeLists.txt').read_text())

    def test_instance_state_keeps_clicks_data_rows_and_platform_skip(self):
        source = (ROOT / 'tests/tst_instance_state.cpp').read_text()
        for token in ('InstanceStateSignalRecorder', 'recordClicked', 'clickedConnection',
                      'owned_input_lifecycle:', 'menu_selected_uses_press_fill:',
                      'ListItem-', 'ContextMenuItem', 'HierarchyItem',
                      'LVRS_STATE_CAPTURE_DIR', 'lvrs_ctest::skip(',
                      'QEvent::MouseButtonPress', 'QEvent::MouseButtonRelease',
                      'QEvent::MouseMove', 'QEvent::KeyPress', 'QEvent::KeyRelease',
                      'figma-instance-state.json', 'hasFocusOutline'):
            self.assertTrue(token in source, f'missing {token}')
        self.assertTrue('mouseMove(window, center);\n    mouseClick(window, center);' in source)
        cmake = (ROOT / 'tests/CMakeLists.txt').read_text()
        self.assertTrue('LVRSTests_instance_state.${case}' in cmake)

    def test_list_composites_keep_variant_signal_payload_and_capture(self):
        source = (ROOT / 'tests/tst_list_composites.cpp').read_text()
        for token in ('ListCompositeSignalRecorder', 'recordAction', 'recordEdit',
                      'actionEvents', 'editEvents', 'QQuickItemGrabResult::ready',
                      'captureConnection', 'QObject::disconnect(captureConnection)',
                      'figma_variants:', 'LVRS_LIST_DIAGNOSTIC_CAPTURE',
                      'LVRS_LIST_CAPTURE_DIR', 'LVRS_TEST_SOURCE_DIR',
                      'QEvent::MouseButtonPress', 'QEvent::MouseButtonRelease',
                      'QEvent::KeyPress', 'QEvent::KeyRelease'):
            self.assertTrue(token in source, f'missing {token}')
        self.assertFalse('QFINDTESTDATA' in source)
        cmake = (ROOT / 'tests/CMakeLists.txt').read_text()
        self.assertTrue('LVRSTests_list_composites.${case}' in cmake)

    def test_colorpicker_keeps_signal_payloads_and_image_completion(self):
        source = (ROOT / 'tests/tst_colorpicker.cpp').read_text()
        for token in ('content_variants:', 'const int heights[] = {461, 437, 437, 361, 413, 439}',
                      'ColorPickerSignalRecorder', 'recordEdited', 'recordAccepted',
                      'recordCanceled', 'recordEyedropper', 'editedColors', 'acceptedColors',
                      'QQuickItemGrabResult::ready', 'captureConnection',
                      'QObject::disconnect(captureConnection)', 'TouchSequence',
                      'QEvent::MouseButtonPress', 'QEvent::MouseButtonRelease',
                      'QEvent::KeyPress', 'QEvent::KeyRelease', 'LVRS_COLORPICKER_CAPTURE_DIR'):
            self.assertTrue(token in source, f'missing {token}')
        cmake = (ROOT / 'tests/CMakeLists.txt').read_text()
        self.assertTrue('LVRSTests_colorpicker.${case}' in cmake)

    def test_platform_integration_keeps_platform_conditions_and_resize_events(self):
        source = (ROOT / 'tests/tst_platform_integration.cpp').read_text()
        for token in ('#if defined(Q_OS_MACOS)', 'lvrs_ctest::skip(',
                      'QEvent::MouseMove', 'QEvent::MouseButtonRelease',
                      'requestSystemResizeAt', 'requestSystemResize(&window',
                      'setMinimumSize', 'platform.runtimeProfiles()',
                      'mobile_theme_scale_contract', 'CTEST_CHECK_MESSAGE'):
            self.assertTrue(token in source, f'missing {token}')
        self.assertFalse('QFINDTESTDATA' in source, 'fixture lookup must not require QtTest')
        self.assertTrue('LVRS_TEST_SOURCE_DIR' in source)
        cmake = (ROOT / 'tests/CMakeLists.txt').read_text()
        self.assertTrue('LVRSTests_platform_integration.${case}' in cmake)
        self.assertTrue('SKIP_RETURN_CODE 77' in cmake)

    def test_slider_keeps_variant_matrix_signal_input_and_rendering(self):
        source = (ROOT / 'tests/tst_slider.cpp').read_text()
        expected = [(target, kind, size, state)
                    for target in ('macos', 'ios') for kind in range(7)
                    for size in range(4) for state in range(1, 5)]
        self.assertEqual(len(expected), 224)
        for token in ('struct SliderVariant', 'sliderVariants()', 'figma_variants:',
                      'SliderSignalRecorder', 'recordMoved', 'movedConnection',
                      'QQuickItemGrabResult::ready', 'captureConnection',
                      'QObject::disconnect(captureConnection)', 'TouchSequence',
                      'QEvent::MouseButtonPress', 'QEvent::MouseButtonRelease',
                      'QEvent::MouseMove', 'QEvent::KeyPress', 'QEvent::KeyRelease',
                      'LVRS_SLIDER_CAPTURE_DIR', 'slider gallery pixel counts',
                      'matchesAccentPixel', 'expectedAccent', '5000)'):
            self.assertTrue(token in source, f'missing {token}')
        cmake = (ROOT / 'tests/CMakeLists.txt').read_text()
        self.assertTrue('LVRSTests_slider.${case}' in cmake)

    def test_card_keeps_variant_matrix_signals_and_rendering(self):
        source = (ROOT / 'tests/tst_card.cpp').read_text()
        expected = tuple((mobile, kind, size, detail, state)
                         for mobile in (False, True) for kind in range(8)
                         for size in range(3 if kind == 0 else 1)
                         for detail in range(2 if kind == 0 else 1)
                         for state in range(1, (1 if kind == 1 else 3) + 1))
        self.assertEqual(len(expected), 74)
        for token in ('struct CardVariant', 'cardVariants()', 'figma_contract:',
                      'CardSignalRecorder', 'recordClicked', 'recordMenu', 'recordAction',
                      'QQuickItemGrabResult::ready', 'captureConnection',
                      'QObject::disconnect(captureConnection)', 'LVRS_CARD_CAPTURE_DIR',
                      'QEvent::MouseButtonPress', 'QEvent::MouseButtonRelease',
                      'QEvent::KeyPress', 'QEvent::KeyRelease', '5000)'):
            self.assertTrue(token in source, f'missing {token}')
        cmake = (ROOT / 'tests/CMakeLists.txt').read_text()
        self.assertTrue('LVRSTests_card.${case}' in cmake)

    def test_toggle_switch_keeps_signal_data_input_and_cleanup(self):
        source = (ROOT / 'tests/tst_toggle_switch.cpp').read_text()
        for token in ('ToggleSignalRecorder', 'recordToggled', 'toggledConnection',
                      'off-to-on', 'on-to-off', 'dataTag', 'TouchSequence',
                      'QEvent::MouseButtonPress', 'QEvent::MouseButtonRelease',
                      'QEvent::MouseMove', 'QEvent::KeyPress', 'QEvent::KeyRelease',
                      'qScopeGuard', 'runner.run(name', 'QObject::disconnect(connection)',
                      'LVRS_TOGGLE_CAPTURE_DIR', '300)', '200)'):
            self.assertIn(token, source)

    def test_mobile_navigation_preserves_signal_input_and_timing(self):
        source = (ROOT / 'tests/tst_mobile_navigation.cpp').read_text()
        for token in ('NavigationSignalRecorder', 'recordActivated', 'recordSearch',
                      'activatedConnection', 'searchConnection', 'TouchSequence',
                      'QEvent::MouseButtonPress', 'QEvent::MouseButtonRelease',
                      'QEvent::KeyPress', 'QEvent::KeyRelease', 'lvrs_ctest::skip(',
                      'LVRS_MOBILE_NAVIGATION_CAPTURE_PATH', '250)', '1000)'):
            self.assertIn(token, source)

    def test_tabs_preserves_activation_values_touch_and_capture_skip(self):
        source = (ROOT / 'tests/tst_tabs.cpp').read_text()
        for token in ('TabSignalRecorder', 'recordActivated', 'activatedConnection',
                      'activated.values.first()', 'TouchSequence', 'lvrs_ctest::skip(',
                      'LVRS_TAB_ICON_SCREENSHOT_PATH', 'LVRS_TABS_SCREENSHOT_PATH'):
            self.assertIn(token, source)
        touch = (ROOT / 'tests/QtGuiTouch.h').read_text()
        self.assertIn('QtGui/qtestsupport_gui.h', touch)
        self.assertNotIn('<QtTest', touch)
        touch_test = (ROOT / 'tests/ctest_touch_support.cpp').read_text()
        for state in ('Pressed', 'Updated', 'Released'):
            self.assertIn(f'QEventPoint::State::{state}', touch_test)

    def test_materials_keeps_capture_popup_and_renderer_skip(self):
        source = (ROOT / 'tests/tst_materials.cpp').read_text()
        for token in ('QQuickItemGrabResult::ready', 'captureConnection',
                      'QObject::disconnect(captureConnection)', 'MaterialPopupRecorder',
                      'recordOpening', 'recordClosing', 'openingConnection',
                      'closingConnection', 'lvrs_ctest::skip(', 'QSGRendererInterface::Software',
                      'grainStats', 'qFuzzyCompare', 'processFor(180)'):
            self.assertIn(token, source)
        cmake = (ROOT / 'tests/CMakeLists.txt').read_text()
        self.assertIn('SKIP_RETURN_CODE 77', cmake)

    def test_sheet_keeps_capture_dismissal_and_drag_observations(self):
        source = (ROOT / 'tests/tst_sheet.cpp').read_text()
        for token in ('QQuickItemGrabResult::ready', 'captureConnection',
                      'QObject::disconnect(captureConnection)', 'SheetSignalRecorder',
                      'recordClosed', 'closedConnection', 'destroyedConnection',
                      'destroyedCount', 'QEvent::MouseButtonPress',
                      'QEvent::MouseMove', 'QEvent::MouseButtonRelease',
                      'QEvent::KeyPress', 'QEvent::KeyRelease', 'qFuzzyCompare'):
            self.assertIn(token, source)

    def test_backend_io_records_async_results_and_isolates_workspaces(self):
        source = (ROOT / 'tests/tst_backend_io.cpp').read_text()
        for token in ('class BackendObservations', 'Backend::asyncRequestFinished',
                      'Backend::asyncRequestQueued', 'Backend::asyncJobsInFlightChanged',
                      'QVariant::fromValue(requestId)', 'QVariant::fromValue(elapsedMs)',
                      'observations.finished', 'finishedArgsFor', 'cancelAsyncRequest',
                      'QStringLiteral("test-cancel")', 'QStringLiteral("canceled")',
                      'ctest-backend-XXXXXX', 'processFor(220)'):
            self.assertIn(token, source)
        for case in ('backend_file_roundtrip_and_errors',
                     'backend_error_signal_and_directory_idempotence',
                     'backend_async_concurrency_and_io_contract',
                     'backend_async_delay_distribution_and_read_cache_contract',
                     'backend_p1_scheduler_backpressure_cancel_and_cache_capacity_contract'):
            body = source.split(f'void BackendIoTests::{case}()', 1)[1].split('\nvoid BackendIoTests::', 1)[0]
            self.assertLess(body.index('QTemporaryDir tempDir'), body.index('Backend backend;'))

    def test_primary_color_preserves_warning_and_image_completion(self):
        source = (ROOT / 'tests/tst_primary_color.cpp').read_text()
        for token in ('Binding loop detected', 'bindingLoopObserved',
                      'qInstallMessageHandler', 'QQuickItemGrabResult::ready',
                      'captureConnection', 'QObject::disconnect(captureConnection)',
                      'QColor("#80A571E6")', 'QByteArray("AppBootstrapWindow")',
                      'QByteArray("AppShell")', 'LVRS_TEST_SOURCE_DIR'):
            self.assertIn(token, source)

    def test_render_quality_keeps_signal_counts_and_isolated_cache(self):
        source = (ROOT / 'tests/tst_render_quality.cpp').read_text()
        for name in ('enabled', 'scale', 'scene', 'msaa', 'text', 'frames',
                     'partial', 'batch', 'inactiveDowngrade', 'inactiveMsaa'):
            self.assertIn(f'++{name}Count', source)
            self.assertIn(f'CTEST_CHECK({name}Connection)', source)
        for token in ('ctest-render-quality-XXXXXX', 'pipeline.cache',
                      'resolveTextureSource', 'sampleFrameTime', 'qScopeGuard'):
            self.assertIn(token, source)

    def test_text_editor_retains_signal_payloads_and_workspace(self):
        source = (ROOT / 'tests/tst_text_editor.cpp').read_text()
        for token in ('recordDocumentEdited', 'recordReadFinished', 'recordSyncFinished',
                      'documentConnection', 'readConnection', 'syncConnection',
                      'documentEvents.takeFirst()', 'readEvents.takeFirst()',
                      'syncEvents', 'ctest-text-editor-XXXXXX',
                      'QEvent::KeyPress', 'QEvent::KeyRelease',
                      '#include "tst_text_editor.moc"'):
            self.assertIn(token, source)
        self.assertEqual(source.count('Q_OBJECT'), 1)
        cmake = (ROOT / 'tests/CMakeLists.txt').read_text()
        self.assertIn('LVRSTests_text_editor.${case}', cmake)

    def test_avatar_preserves_capture_signals_and_data_rows(self):
        source = (ROOT / 'tests/tst_avatar.cpp').read_text()
        for token in ('QQuickItemGrabResult::ready', 'captureConnection',
                      'QObject::disconnect(captureConnection)',
                      'centered_crop_pixels:', 'image_formats:',
                      'for (bool portrait : {false, true})',
                      'for (int diameter : {24, 40, 64, 96})',
                      'QByteArray("png")', 'QByteArray("jpeg")', 'QByteArray("webp")',
                      'QByteArray("bmp")', 'QByteArray("svg")', 'QByteArray("gif")',
                      'QDir::cleanPath', 'ctest-avatar-XXXXXX'):
            self.assertIn(token, source)
        self.assertNotRegex(source, r'\b(?:QTest|QSignalSpy)\b|\bQTEST_|<QtTest|\bQ(?:VERIFY|COMPARE|TRY|FETCH|SKIP)\w*')
        cmake = (ROOT / 'tests/installed-consumer/CMakeLists.txt').read_text()
        self.assertNotRegex(cmake, r'Qt6::Test|find_package\(Qt6[^\n]*\bTest\b')
        self.assertIn('LVRS_TEST_SOURCE_DIR=', cmake)
        self.assertIn('uifw_add_test(LVRSTests_avatar tst_avatar.cpp)',
                      (ROOT / 'tests/CMakeLists.txt').read_text())

    def test_code_editor_keeps_native_event_matrix(self):
        source = (ROOT / 'tests/tst_code_editor.cpp').read_text()
        for token in ('QEvent::MouseButtonPress', 'QEvent::MouseButtonRelease',
                      'QEvent::MouseButtonDblClick', 'QEvent::MouseMove',
                      'QEvent::KeyPress', 'QEvent::KeyRelease',
                      'QInputMethodEvent preeditEvent', 'QInputMethodEvent commitEvent',
                      'doubleClickSelection', 'selectedText', 'submitCount'):
            self.assertIn(token, source)
        double_click = source.split('void mouseDoubleClick', 1)[1].split('\n}', 1)[0]
        self.assertRegex(double_click, re.compile(
            r'mouseClick\(window, point\).*QEvent::MouseButtonPress.*'
            r'QEvent::MouseButtonDblClick.*QEvent::MouseButtonRelease', re.DOTALL))
        self.assertIn('lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "")', source)
        cmake = (ROOT / 'tests/CMakeLists.txt').read_text()
        self.assertIn('LVRSTests_code_editor.${case}', cmake)

    def test_native_window_blur_keeps_cocoa_cases_and_skip_semantics(self):
        source = (ROOT / 'tests/tst_nativewindowblur.mm').read_text()
        for case in ('lifecycle_and_foreground', 'explicit_native_blur_remains_opt_in',
                     'unified_titlebar_controls', 'unified_titlebar_survives_native_update',
                     'solid_chrome_disables_background_movement'):
            self.assertIn(f'runner.run("{case}"', source)
        for token in ('lvrs_ctest::skip(', 'NSWindowDidEnterFullScreenNotification',
                      'NSWindowDidExitFullScreenNotification', 'QEvent::MouseButtonPress',
                      'QEvent::MouseButtonRelease', 'CTEST_WAIT_VERIFY(enteredFullScreen, 10000)',
                      'CTEST_WAIT_VERIFY(exitedFullScreen, 10000)'):
            self.assertIn(token, source)
        self.assertNotRegex(source, r'\b(?:QTest|QSignalSpy)\b|\bQTEST_|<QtTest|\bQ(?:VERIFY|COMPARE|TRY|SKIP)\w*')
        cmake = (ROOT / 'tests/CMakeLists.txt').read_text()
        self.assertIn('uifw_add_test(LVRSTests_nativewindowblur tst_nativewindowblur.mm)', cmake)
        self.assertIn('set_tests_properties(LVRSTests_nativewindowblur PROPERTIES SKIP_RETURN_CODE 77)', cmake)

    def test_document_model_retains_signal_and_model_contracts(self):
        source = (ROOT / 'tests/tst_text_document_model.cpp').read_text()
        for token in ('fileLoaded', 'modelReset', 'rowsInserted', 'rowsRemoved',
                      'dataChanged', 'lineStorageChanged', 'filePathChanged',
                      'validateModel', 'CTEST_WAIT_COMPARE_TIMEOUT',
                      '../resources/ctest-document-XXXXXX'):
            self.assertIn(token, source)
        self.assertNotIn('QAbstractItemModelTester', source)

    def test_catalog_preserves_capture_skip_and_window_exposure(self):
        source = (ROOT / 'tests/tst_catalog.cpp').read_text()
        self.assertIn('lvrs_ctest::skip(', source)
        self.assertIn('window->isExposed()', source)
        self.assertIn('processFor(600)', source)
        cmake = (ROOT / 'tests/CMakeLists.txt').read_text()
        self.assertIn('set_tests_properties(LVRSTests_catalog PROPERTIES SKIP_RETURN_CODE 77)', cmake)
        support = (ROOT / 'tests/ctest_support.cpp').read_text()
        self.assertIn('skipped.result() != 77', support)
        self.assertIn('mixed.result() != 0', support)

    def test_windows_qml_link_keeps_its_actual_import_check(self):
        source = (ROOT / 'tests/tst_windows_qml_link.cpp').read_text()
        self.assertIn('QGuiApplication application(argc, argv)', source)
        self.assertIn('import LVRS 1.0', source)
        self.assertIn('component.create()', source)
        self.assertIn('return object ? 0 : 1', source)

    def test_svg_signal_counts_and_temporary_resources_are_preserved(self):
        source = (ROOT / 'tests/tst_svg_manager.cpp').read_text()
        for name in ('error', 'vector', 'minimum', 'maximum', 'cache', 'revision'):
            self.assertIn(f'++{name}Count', source)
            self.assertIn(f'CTEST_CHECK({name}Connection)', source)
        self.assertIn('../resources/ctest-svg-XXXXXX', source)
        self.assertIn('workspace.filePath("svgmanager-XXXXXX.svg")', source)
        self.assertIn('icons.size(), 2846', source)

    def test_visual_regression_preserves_capture_and_diff(self):
        source = (ROOT / 'tests/tst_visual_regression.cpp').read_text()
        for token in ('QQuickItemGrabResult::ready', 'readyConnection', 'LVRS_UPDATE_GOLDEN',
                      'LVRS_VISUAL_CAPTURE_PATH', 'visual_baseline_scene.diff.png',
                      'setAutoRemove(false)', '../resources/ctest-visual-XXXXXX'):
            self.assertIn(token, source)

    def test_cases_are_registered_without_qttest(self):
        for suite, (_, cases) in CASES.items():
            with self.subTest(suite=suite):
                source = (ROOT / f"tests/tst_{suite}.cpp").read_text()
                for case in cases:
                    registration = 'runCase' if suite == 'toggle_switch' else 'runner.run'
                    self.assertIn(f'{registration}("{case}"', source)
                self.assertNotRegex(source, r"\b(?:QTest|QSignalSpy|QAbstractItemModelTester)\b|\bQTEST_|<QtTest|\bQ(?:VERIFY|COMPARE|TRY|FETCH|SKIP|FAIL)\w*")

    def test_targets_do_not_require_qttest(self):
        cmake = (ROOT / "tests/CMakeLists.txt").read_text()
        self.assertNotIn('Qt6::Test', cmake)
        self.assertNotIn('NO_QT_TEST', cmake)
        self.assertNotRegex((ROOT / 'CMakeLists.txt').read_text(),
                            r'find_package\(Qt6[^\n]*\bTest\b')
        for suite in CASES:
            self.assertRegex(cmake, r"uifw_add_test\(LVRSTests_" + suite + r" tst_" + suite + r'\.cpp(?:\s+LABELS\s+"[^"]+")?\)')

    def test_event_wait_support_and_source_data_lookup(self):
        source = (ROOT / "tests/tst_examples.cpp").read_text()
        self.assertIn("findExampleFile(", source)
        self.assertIn("LVRS_TEST_SOURCE_DIR", source)
        self.assertNotIn("QFINDTESTDATA", source)
        support = (ROOT / "tests/ctest_event_support.cpp").read_text()
        for case in ("immediate_predicate", "queued_callback", "timeout_is_reported", "deferred_deletion_is_flushed"):
            self.assertIn(f'runner.run("{case}"', support)
        cmake = (ROOT / "tests/CMakeLists.txt").read_text()
        self.assertIn("uifw_add_test(LVRSTests_ctest_event_support ctest_event_support.cpp)", cmake)

    def test_permission_signal_is_connected_and_counted(self):
        source = (ROOT / "tests/tst_permission_sequencer.cpp").read_text()
        self.assertIn("CTEST_CHECK(historyConnection)", source)
        self.assertIn("CTEST_COMPARE(historyCount, 1)", source)
        self.assertIn("CTEST_COMPARE(historyCount, 2)", source)

    def test_registrar_fixture_keeps_qt_metaobjects(self):
        source = (ROOT / "tests/tst_qml_type_registrar.cpp").read_text()
        self.assertEqual(source.count("Q_OBJECT"), 2)
        self.assertIn('#include "tst_qml_type_registrar.moc"', source)

    def test_performance_signal_counts_and_resource_directory(self):
        for suite in ("performance_gate", "soak_runtime"):
            source = (ROOT / f"tests/tst_{suite}.cpp").read_text()
            self.assertIn("CTEST_CHECK(finishedConnection)", source)
            self.assertIn("++finishedCount", source)
            self.assertIn("CTEST_WAIT_COMPARE_TIMEOUT(finishedCount", source)
        source = (ROOT / "tests/tst_soak_runtime.cpp").read_text()
        self.assertIn('"../resources/ctest-soak-XXXXXX"', source)

    def test_app_entry_temp_files_are_in_resources(self):
        source = (ROOT / "tests/tst_app_entry.cpp").read_text()
        self.assertIn('"../resources/ctest-app-entry-XXXXXX"', source)
        self.assertNotIn("QTemporaryDir tempDir;", source)

if __name__ == "__main__":
    unittest.main()
