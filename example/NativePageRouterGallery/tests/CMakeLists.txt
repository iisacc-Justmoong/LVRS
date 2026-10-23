add_test(NAME LVRS.native_page_router_gallery COMMAND LVRSNativePageRouterGallery --self-test)
set_tests_properties(LVRS.native_page_router_gallery PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
