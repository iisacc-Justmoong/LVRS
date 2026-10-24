add_test(NAME LVRS.native_tooltip_gallery COMMAND LVRSNativeTooltipGallery --self-test)
set_tests_properties(LVRS.native_tooltip_gallery PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE)
