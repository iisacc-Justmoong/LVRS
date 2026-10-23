add_test(NAME LVRS.native_navigation_gallery COMMAND LVRSNativeNavigationGallery --self-test)
set_tests_properties(LVRS.native_navigation_gallery PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
