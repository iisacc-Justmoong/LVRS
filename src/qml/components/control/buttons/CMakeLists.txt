set(LVRS_CONTROL_BUTTON_QML_FILES
    src/qml/components/control/buttons/AbstractButton.qml
    src/qml/components/control/buttons/PushButton.qml
    src/qml/components/control/buttons/HelpButton.qml
    src/qml/components/control/buttons/ColorPickerButton.qml
    src/qml/components/control/buttons/DropdownButton.qml
    src/qml/components/control/buttons/ButtonMethodRegistry.qml
    src/qml/components/control/buttons/LabelMenuButton.qml
    src/qml/components/control/buttons/IconMenuButton.qml
    src/qml/components/control/buttons/LabelButton.qml
    src/qml/components/control/buttons/IconButton.qml
    src/qml/components/control/buttons/ComboBox.qml
    src/qml/components/control/buttons/Stepper.qml
    src/qml/components/control/buttons/LabelSegmentedControl.qml
    src/qml/components/control/buttons/IconSegmentedControl.qml
)

set(_lvrs_control_button_iconset_dir
    "${CMAKE_CURRENT_LIST_DIR}/../../../../../resources/iconset"
)
file(GLOB_RECURSE _lvrs_control_button_svg_icons
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES FALSE
    "${_lvrs_control_button_iconset_dir}/*.svg"
)
list(FILTER _lvrs_control_button_svg_icons EXCLUDE REGEX "\\.DS_Store$")

if(NOT _lvrs_control_button_svg_icons)
    message(FATAL_ERROR
        "LVRS control button icons are missing. "
        "Expected at least one SVG under resources/iconset/."
    )
endif()

unset(_lvrs_control_button_svg_icons)
unset(_lvrs_control_button_iconset_dir)

# Icon resources are collected globally in src/qml/CMakeLists.txt via GLOB_RECURSE.
set(LVRS_CONTROL_BUTTON_RESOURCE_FILES)
