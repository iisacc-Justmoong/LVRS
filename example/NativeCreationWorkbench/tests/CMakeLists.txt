add_test(NAME LVRS.native_creation_workbench COMMAND LVRSNativeCreationWorkbench --self-test)
set_tests_properties(LVRS.native_creation_workbench PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
