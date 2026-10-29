# Provide stable LVRS public include paths for responsibility-grouped sources.
foreach(_area IN ITEMS Overlays Components Hierarchy Lists Tables Text Models Navigation Monitoring Platform Styling Media Controls)
    file(GLOB _headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/${_area}/*.h")
    foreach(_header IN LISTS _headers)
        get_filename_component(_name "${_header}" NAME)
        if(DEFINED _lvrs_native_header_${_name})
            message(FATAL_ERROR "Duplicate LVRS native header basename: ${_name}")
        endif()
        set("_lvrs_native_header_${_name}" "${_header}")
        file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/LVRS/${_name}"
            CONTENT "#pragma once\n#include \"${_header}\"\n")
    endforeach()
endforeach()
