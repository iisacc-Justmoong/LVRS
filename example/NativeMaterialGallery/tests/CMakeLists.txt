add_test(NAME LVRS.native_material_gallery COMMAND LVRSNativeMaterialGallery --self-test)
set_tests_properties(LVRS.native_material_gallery PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
