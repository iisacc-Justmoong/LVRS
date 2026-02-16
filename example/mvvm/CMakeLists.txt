cmake_minimum_required(VERSION 3.21)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(LVRSExampleMVVM LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Svg Network)
    qt_standard_project_setup()
endif()

if(NOT TARGET LVRSCore)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. backend_build)
endif()

qt_add_executable(LVRSExampleMVVM
    main.cpp
    backend/ExampleBootstrap.cpp
    backend/ExampleModel.cpp
    backend/ExampleViewModel.cpp
)

qt_add_qml_module(LVRSExampleMVVM
    URI ExampleMVVM
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        qml/Main.qml
)

target_link_libraries(LVRSExampleMVVM
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)

lvrs_configure_qml_app(LVRSExampleMVVM)
lvrs_configure_example_target(LVRSExampleMVVM)

target_include_directories(LVRSExampleMVVM
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(run_LVRSExampleMVVM
    COMMAND $<TARGET_FILE:LVRSExampleMVVM>
    DEPENDS LVRSExampleMVVM
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
)
