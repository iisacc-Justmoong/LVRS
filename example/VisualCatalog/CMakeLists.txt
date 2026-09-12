cmake_minimum_required(VERSION 3.21)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/LVRSHelpers.cmake")

lvrs_add_example_app(
    TARGET LVRSExampleVisualCatalog
    URI ExampleVisualCatalog
    SOURCES
        main.cpp
    QML_FILES
        qml/CatalogRegistry.qml
        qml/CatalogMotion.js
        qml/MotionInspector.qml
        qml/MotionLab.qml
        qml/CardGallery.qml
        qml/SliderGallery.qml
        qml/ColorPickerGallery.qml
        qml/SheetGallery.qml
        qml/TooltipGallery.qml
        qml/MaterialGallery.qml
        qml/FigmaParityGallery.qml
        qml/Main.qml
    RESOURCES
        assets/cards/architecture.png
        assets/cards/landscape.png
        assets/cards/document.png
)
