option(LVRS_BUILD_LEGACY_EXAMPLES "Build legacy desktop example apps" ON)

set(_lvrs_host_example_targets "")

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_subdirectory(iOSHello)
    list(APPEND _lvrs_host_example_targets LVRSExampleIOSHello)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    add_subdirectory(AndroidHello)
    list(APPEND _lvrs_host_example_targets LVRSExampleAndroidHello)
else()
    if(LVRS_BUILD_LEGACY_EXAMPLES)
        add_subdirectory(VisualCatalog)
        add_subdirectory(EventListener)
        add_subdirectory(mvvm)
        add_subdirectory(TypingPractice)
        list(APPEND _lvrs_host_example_targets
            LVRSExampleVisualCatalog
            LVRSExampleEventListener
            LVRSExampleMVVM
            LVRSExampleTypingPractice
        )
    endif()
    add_subdirectory(iOSHello)
    add_subdirectory(AndroidHello)
    list(APPEND _lvrs_host_example_targets
        LVRSExampleIOSHello
        LVRSExampleAndroidHello
    )
endif()

if(_lvrs_host_example_targets)
    add_custom_target(lvrs_host_examples_all
        DEPENDS ${_lvrs_host_example_targets}
    )
endif()
