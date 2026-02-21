set(LVRS_CONTROL_BUTTON_QML_FILES
    qml/components/control/buttons/AbstractButton.qml
    qml/components/control/buttons/LabelMenuButton.qml
    qml/components/control/buttons/IconMenuButton.qml
    qml/components/control/buttons/LabelButton.qml
    qml/components/control/buttons/IconButton.qml
    qml/components/control/buttons/LabelSegmentedControl.qml
    qml/components/control/buttons/IconSegmentedControl.qml
)

set(LVRS_CONTROL_BUTTON_REQUIRED_ICON_NAMES
    viewMoreSymbolicDefault
    viewMoreSymbolicBorderless
    viewMoreSymbolicDisabled
    panDownSymbolicDefault
    panDownSymbolicAccent
    panDownSymbolicBorderless
    panDownSymbolicDisabled
)

set(_lvrs_control_button_iconset_dir
    "${CMAKE_CURRENT_LIST_DIR}/../../../../resources/iconset"
)
set(_lvrs_control_button_missing_icons "")

foreach(_lvrs_icon_name IN LISTS LVRS_CONTROL_BUTTON_REQUIRED_ICON_NAMES)
    if(NOT EXISTS "${_lvrs_control_button_iconset_dir}/${_lvrs_icon_name}.svg")
        list(APPEND _lvrs_control_button_missing_icons "${_lvrs_icon_name}.svg")
    endif()
endforeach()

if(_lvrs_control_button_missing_icons)
    list(JOIN _lvrs_control_button_missing_icons ", " _lvrs_control_button_missing_icons_joined)
    message(FATAL_ERROR
        "LVRS control button required icons are missing in resources/iconset: "
        "${_lvrs_control_button_missing_icons_joined}"
    )
endif()

unset(_lvrs_icon_name)
unset(_lvrs_control_button_missing_icons)
unset(_lvrs_control_button_missing_icons_joined)
unset(_lvrs_control_button_iconset_dir)

# Icon resources are collected globally in qml/CMakeLists.txt via GLOB_RECURSE.
set(LVRS_CONTROL_BUTTON_RESOURCE_FILES)
