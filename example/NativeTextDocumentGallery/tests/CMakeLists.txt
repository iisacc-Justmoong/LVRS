add_test(NAME LVRS.native_text_document_gallery COMMAND LVRSNativeTextDocumentGallery --self-test "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(LVRS.native_text_document_gallery PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
