cmake_minimum_required(VERSION 3.21)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(LVRSExampleEventListener LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Svg Network)
    qt_standard_project_setup()
endif()

if(NOT TARGET LVRSCore)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. backend_build)
endif()

qt_add_executable(LVRSExampleEventListener
    main.cpp
)

qt_add_qml_module(LVRSExampleEventListener
    URI ExampleEventListener
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        Main.qml
        BasicClick.qml
        HoverCard.qml
        PressRelease.qml
        RightClick.qml
        KeyPress.qml
        MultiTrigger.qml
)

target_link_libraries(LVRSExampleEventListener
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
)

lvrs_configure_qml_app(LVRSExampleEventListener)
lvrs_configure_example_target(LVRSExampleEventListener)

add_custom_target(run_LVRSExampleEventListener
    COMMAND $<TARGET_FILE:LVRSExampleEventListener>
    DEPENDS LVRSExampleEventListener
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    USES_TERMINAL
)
