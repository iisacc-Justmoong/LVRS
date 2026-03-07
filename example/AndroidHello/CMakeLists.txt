cmake_minimum_required(VERSION 3.21)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/LVRSHelpers.cmake")

lvrs_add_example_app(
    TARGET LVRSExampleAndroidHello
    URI ExampleAndroidHello
    SOURCES
        main.cpp
    QML_FILES
        qml/Main.qml
)
