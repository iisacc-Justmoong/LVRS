set(LVRS_SURFACE_QML_FILES
    qml/components/surfaces/AppCard.qml
    qml/components/surfaces/Card.qml
    qml/components/surfaces/CardImageContent.qml
    qml/components/surfaces/CardInformationContent.qml
    qml/components/surfaces/Alert.qml
    qml/components/surfaces/AlertButton.qml
    qml/components/surfaces/Modal.qml
)

set_source_files_properties(
    qml/components/surfaces/CardImageContent.qml
    qml/components/surfaces/CardInformationContent.qml
    PROPERTIES QT_QML_INTERNAL_TYPE TRUE
)

# Keep this component family beside the module qmldir so the QML cache and
# runtime resolve internal types to the same URLs in shared and installed builds.
set_source_files_properties(qml/components/surfaces/Card.qml
    PROPERTIES QT_RESOURCE_ALIAS Card.qml)
set_source_files_properties(qml/components/surfaces/CardImageContent.qml
    PROPERTIES QT_RESOURCE_ALIAS CardImageContent.qml)
set_source_files_properties(qml/components/surfaces/CardInformationContent.qml
    PROPERTIES QT_RESOURCE_ALIAS CardInformationContent.qml)
