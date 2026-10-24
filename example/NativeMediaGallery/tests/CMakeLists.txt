add_test(NAME LVRS.native_media_gallery COMMAND LVRSNativeMediaGallery --self-test)
set_tests_properties(LVRS.native_media_gallery PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
