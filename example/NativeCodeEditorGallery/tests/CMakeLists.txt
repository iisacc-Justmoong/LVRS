add_test(NAME LVRS.native_code_editor_gallery COMMAND LVRSNativeCodeEditorGallery --self-test "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(LVRS.native_code_editor_gallery PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
