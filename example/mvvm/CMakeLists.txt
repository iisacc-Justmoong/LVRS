cmake_minimum_required(VERSION 3.21)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/LVRSHelpers.cmake")

lvrs_add_example_app(
    TARGET LVRSExampleMVVM
    URI ExampleMVVM
    SOURCES
        main.cpp
        backend/ExampleBootstrap.cpp
        backend/ExampleModel.cpp
        backend/ExampleViewModel.cpp
    QML_FILES
        qml/Main.qml
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
)
