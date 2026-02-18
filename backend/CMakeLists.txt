set(LVRS_BACKEND_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/io/backend.cpp
    ${CMAKE_CURRENT_LIST_DIR}/fonts/fontpolicy.cpp
    ${CMAKE_CURRENT_LIST_DIR}/runtime/renderquality.cpp
    ${CMAKE_CURRENT_LIST_DIR}/runtime/runtimeevents.cpp
    ${CMAKE_CURRENT_LIST_DIR}/platform/platforminfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/runtime/renderingmonitor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/runtime/vulkanbootstrap.cpp
    ${CMAKE_CURRENT_LIST_DIR}/runtime/appbootstrap.cpp
    ${CMAKE_CURRENT_LIST_DIR}/runtime/appentry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/navigation/pagemonitor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/navigation/routematcher.cpp
    ${CMAKE_CURRENT_LIST_DIR}/navigation/viewstatetracker.cpp
    ${CMAKE_CURRENT_LIST_DIR}/state/appstate.cpp
    ${CMAKE_CURRENT_LIST_DIR}/state/viewmodelregistry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/runtime/debuglogger.cpp
    ${CMAKE_CURRENT_LIST_DIR}/graphics/svgmanager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/text/textmarkup.cpp
)

set(LVRS_BACKEND_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/io/backend.h
    ${CMAKE_CURRENT_LIST_DIR}/fonts/fontpolicy.h
    ${CMAKE_CURRENT_LIST_DIR}/runtime/renderquality.h
    ${CMAKE_CURRENT_LIST_DIR}/runtime/runtimeevents.h
    ${CMAKE_CURRENT_LIST_DIR}/platform/platforminfo.h
    ${CMAKE_CURRENT_LIST_DIR}/runtime/renderingmonitor.h
    ${CMAKE_CURRENT_LIST_DIR}/runtime/vulkanbootstrap.h
    ${CMAKE_CURRENT_LIST_DIR}/runtime/appbootstrap.h
    ${CMAKE_CURRENT_LIST_DIR}/runtime/appentry.h
    ${CMAKE_CURRENT_LIST_DIR}/navigation/pagemonitor.h
    ${CMAKE_CURRENT_LIST_DIR}/navigation/routematcher.h
    ${CMAKE_CURRENT_LIST_DIR}/navigation/viewstatetracker.h
    ${CMAKE_CURRENT_LIST_DIR}/state/appstate.h
    ${CMAKE_CURRENT_LIST_DIR}/state/appstate_macros.h
    ${CMAKE_CURRENT_LIST_DIR}/state/viewmodelregistry.h
    ${CMAKE_CURRENT_LIST_DIR}/runtime/debuglogger.h
    ${CMAKE_CURRENT_LIST_DIR}/graphics/svgmanager.h
    ${CMAKE_CURRENT_LIST_DIR}/text/textmarkup.h
)

if(APPLE AND NOT IOS)
    list(APPEND LVRS_BACKEND_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/platform/nativewindowstyle.mm
    )
else()
    list(APPEND LVRS_BACKEND_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/platform/nativewindowstyle.cpp
    )
endif()

list(APPEND LVRS_BACKEND_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/platform/nativewindowstyle.h
)
