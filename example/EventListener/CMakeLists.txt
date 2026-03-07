cmake_minimum_required(VERSION 3.21)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/LVRSHelpers.cmake")

lvrs_add_example_app(
    TARGET LVRSExampleEventListener
    URI ExampleEventListener
    SOURCES
        main.cpp
    QML_FILES
        Main.qml
        BasicClick.qml
        HoverCard.qml
        PressRelease.qml
        RightClick.qml
        KeyPress.qml
        MultiTrigger.qml
)
