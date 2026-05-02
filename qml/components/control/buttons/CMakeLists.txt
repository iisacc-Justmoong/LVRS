set(LVRS_CONTROL_BUTTON_QML_FILES
    qml/components/control/buttons/AbstractButton.qml
    qml/components/control/buttons/ButtonMethodRegistry.qml
    qml/components/control/buttons/LabelMenuButton.qml
    qml/components/control/buttons/IconMenuButton.qml
    qml/components/control/buttons/LabelButton.qml
    qml/components/control/buttons/IconButton.qml
    qml/components/control/buttons/ComboBox.qml
    qml/components/control/buttons/Stepper.qml
    qml/components/control/buttons/LabelSegmentedControl.qml
    qml/components/control/buttons/IconSegmentedControl.qml
)

set(_lvrs_control_button_iconset_dir
    "${CMAKE_CURRENT_LIST_DIR}/../../../../resources/iconset"
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

# Icon resources are collected globally in qml/CMakeLists.txt via GLOB_RECURSE.
set(LVRS_CONTROL_BUTTON_RESOURCE_FILES)
