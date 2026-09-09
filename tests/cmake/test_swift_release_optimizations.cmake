cmake_minimum_required(VERSION 3.21)

set(consumer "${LVRS_BINARY_DIR}/swift-release-consumer")
file(MAKE_DIRECTORY "${consumer}")
file(WRITE "${consumer}/bits.cpp" "int cxxValue() { return 7; }\n")
file(WRITE "${consumer}/main.swift" "print(\"LVRS Swift Release consumer\")\n")
file(WRITE "${consumer}/CMakeLists.txt" "
cmake_minimum_required(VERSION 3.21)
project(LVRSSwiftConsumer LANGUAGES CXX Swift)
include(\"${LVRS_SOURCE_DIR}/cmake/LVRSHelpers.cmake\")
set(LVRS_ENABLE_IPO OFF)
add_library(CxxBits STATIC bits.cpp)
lvrs_apply_platform_build_optimizations(CxxBits)
add_executable(SwiftConsumer main.swift)
target_link_libraries(SwiftConsumer PRIVATE CxxBits)
lvrs_apply_platform_build_optimizations(SwiftConsumer)
")
execute_process(COMMAND "${CMAKE_COMMAND}" --fresh -G Ninja
    -S "${consumer}" -B "${consumer}/build" -DCMAKE_BUILD_TYPE=Release
    RESULT_VARIABLE configured)
if(NOT configured EQUAL 0)
    message(FATAL_ERROR "Swift Release consumer configure failed")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${consumer}/build"
    RESULT_VARIABLE built)
if(NOT built EQUAL 0)
    message(FATAL_ERROR "C++ and Swift Release optimization flags must compile and link")
endif()
execute_process(COMMAND "${consumer}/build/SwiftConsumer"
    RESULT_VARIABLE ran OUTPUT_VARIABLE output)
if(NOT ran EQUAL 0 OR NOT output MATCHES "LVRS Swift Release consumer")
    message(FATAL_ERROR "Swift Release consumer did not run")
endif()
