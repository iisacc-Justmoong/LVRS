#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "LVRS::LVRS" for configuration "Release"
set_property(TARGET LVRS::LVRS APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LVRS::LVRS PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "Qt6::Core"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libLVRS_arm64-v8a.so"
  IMPORTED_SONAME_RELEASE "libLVRS_arm64-v8a.so"
  )

list(APPEND _cmake_import_check_targets LVRS::LVRS )
list(APPEND _cmake_import_check_files_for_LVRS::LVRS "${_IMPORT_PREFIX}/lib/libLVRS_arm64-v8a.so" )

# Import target "LVRS::LVRSCoreplugin" for configuration "Release"
set_property(TARGET LVRS::LVRSCoreplugin APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(LVRS::LVRSCoreplugin PROPERTIES
  IMPORTED_COMMON_LANGUAGE_RUNTIME_RELEASE ""
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/qt6/qml/LVRS/libqml_LVRS_LVRSCoreplugin_arm64-v8a.so"
  IMPORTED_NO_SONAME_RELEASE "TRUE"
  )

list(APPEND _cmake_import_check_targets LVRS::LVRSCoreplugin )
list(APPEND _cmake_import_check_files_for_LVRS::LVRSCoreplugin "${_IMPORT_PREFIX}/lib/qt6/qml/LVRS/libqml_LVRS_LVRSCoreplugin_arm64-v8a.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
