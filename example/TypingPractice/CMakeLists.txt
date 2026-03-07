cmake_minimum_required(VERSION 3.21)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/LVRSHelpers.cmake")

lvrs_add_example_app(
    TARGET LVRSExampleTypingPractice
    URI ExampleTypingPractice
    SOURCES
        main.cpp
    QML_FILES
        qml/Main.qml
)
