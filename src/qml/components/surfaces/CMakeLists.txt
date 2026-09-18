set(LVRS_SURFACE_QML_FILES
    src/qml/components/surfaces/MaterialSurface.qml
    src/qml/components/surfaces/PanelMaterial.qml
    src/qml/components/surfaces/WindowMaterial.qml
    src/qml/components/surfaces/Popover.qml
    src/qml/components/surfaces/AppCard.qml
    src/qml/components/surfaces/Card.qml
    src/qml/components/surfaces/CardImageContent.qml
    src/qml/components/surfaces/CardInformationContent.qml
    src/qml/components/surfaces/Alert.qml
    src/qml/components/surfaces/AlertButton.qml
    src/qml/components/surfaces/Modal.qml
    src/qml/components/surfaces/Sheet.qml
    src/qml/components/surfaces/Tooltip.qml
)

set_source_files_properties(
    src/qml/components/surfaces/CardImageContent.qml
    src/qml/components/surfaces/CardInformationContent.qml
    PROPERTIES QT_QML_INTERNAL_TYPE TRUE
)

# Keep this component family beside the module qmldir so the QML cache and
# runtime resolve internal types to the same URLs in shared and installed builds.
set_source_files_properties(src/qml/components/surfaces/Card.qml
    PROPERTIES QT_RESOURCE_ALIAS Card.qml)
set_source_files_properties(src/qml/components/surfaces/CardImageContent.qml
    PROPERTIES QT_RESOURCE_ALIAS CardImageContent.qml)
set_source_files_properties(src/qml/components/surfaces/CardInformationContent.qml
    PROPERTIES QT_RESOURCE_ALIAS CardInformationContent.qml)
