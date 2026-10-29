cmake_minimum_required(VERSION 3.31)
project(LVRSInstalledLayoutConsumer LANGUAGES CXX)

find_package(LVRS CONFIG REQUIRED)
add_executable(lvrs_installed_layout_consumer main.cpp)
target_compile_features(lvrs_installed_layout_consumer PRIVATE cxx_std_23)
target_link_libraries(lvrs_installed_layout_consumer PRIVATE LVRS::LVRS)

enable_testing()
add_test(NAME LVRS.installed_layout_consumer COMMAND lvrs_installed_layout_consumer)
