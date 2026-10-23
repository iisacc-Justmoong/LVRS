add_test(NAME LVRS.native_app_surface_gallery COMMAND LVRSNativeAppSurfaces --self-test)
set_tests_properties(LVRS.native_app_surface_gallery PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
