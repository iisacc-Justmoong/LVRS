if(IS_DIRECTORY "${SOURCE_ROOT}/src/LVRS")
    message(FATAL_ERROR "The source directory repeats the project name")
endif()
foreach(_file Styling/Theme.h Styling/Theme.cpp Controls/Widgets.h Controls/Widgets.cpp
        Hierarchy/HierarchyModel.h Hierarchy/HierarchyModel.cpp Lists/List.h Lists/List.cpp
        Tables/Table.h Tables/Table.cpp Navigation/PageRouter.h Navigation/PageRouter.cpp
        Models/ViewModel.h Models/ViewModel.cpp Media/SvgManager.h Media/SvgManager.cpp
        Monitoring/RenderMonitor.h Monitoring/RenderMonitor.cpp Text/CodeEditor.h Text/CodeEditor.cpp)
    if(NOT EXISTS "${SOURCE_ROOT}/src/${_file}")
        message(FATAL_ERROR "Missing responsibility-grouped source: ${_file}")
    endif()
endforeach()
