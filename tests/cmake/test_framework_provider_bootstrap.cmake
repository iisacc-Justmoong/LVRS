cmake_minimum_required(VERSION 3.21)
set(_root "${LVRS_BINARY_DIR}/tests/target-provider")
file(REMOVE_RECURSE "${_root}")
file(MAKE_DIRECTORY "${_root}/provider" "${_root}/child" "${_root}/host provider")
file(WRITE "${_root}/target-toolchain.cmake"
    "set(CMAKE_FIND_ROOT_PATH [==[${_root}/target-sysroot]==])\nset(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)\n")
file(WRITE "${_root}/host provider/iiFileProviderConfig.cmake"
    "message(FATAL_ERROR \"A host provider leaked into the target kit\")\n")
file(WRITE "${_root}/provider/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.21)
project(TargetProvider VERSION 0.7.2 LANGUAGES NONE)
if(BUILD_TESTING OR IIFILEPROVIDER_BUILD_CORE_IO OR NOT IIFILEPROVIDER_BUILD_QT_API)
    message(FATAL_ERROR "Incorrect dependency build features")
endif()
if(NOT CMAKE_ANDROID_ARCH_ABI STREQUAL "forwarded-target-abi")
    message(FATAL_ERROR "The target ABI was lost")
endif()
include(CMakePackageConfigHelpers)
file(WRITE "${CMAKE_BINARY_DIR}/iiFileProviderConfig.cmake"
    "include(CMakeFindDependencyMacro)\nfind_dependency(iiFileProviderNative CONFIG)\nfind_dependency(iiFileProviderCatalog CONFIG)\nfind_dependency(iiFileProviderObjects CONFIG)\nset(TARGET_PROVIDER_FOUND TRUE)\n")
foreach(component IN ITEMS Native Catalog Objects Temporary)
    set(dependencies "")
    if(component STREQUAL "Catalog")
        set(dependencies "include(CMakeFindDependencyMacro)\nfind_dependency(iiFileProviderTemporary CONFIG)\n")
    endif()
    file(WRITE "${CMAKE_BINARY_DIR}/iiFileProvider${component}Config.cmake"
        "${dependencies}set(TARGET_PROVIDER_${component}_FOUND TRUE)\n")
    install(FILES "${CMAKE_BINARY_DIR}/iiFileProvider${component}Config.cmake"
        DESTINATION "lib/cmake/iiFileProvider${component}")
endforeach()
write_basic_package_version_file("${CMAKE_BINARY_DIR}/iiFileProviderConfigVersion.cmake"
    VERSION 0.7.2 COMPATIBILITY SameMinorVersion ARCH_INDEPENDENT)
install(FILES "${CMAKE_BINARY_DIR}/iiFileProviderConfig.cmake"
    "${CMAKE_BINARY_DIR}/iiFileProviderConfigVersion.cmake" DESTINATION lib/cmake/iiFileProvider)
]=])
file(WRITE "${_root}/child/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.21)
project(TargetLVRS LANGUAGES NONE)
find_package(iiFileProvider 0.7.1 CONFIG REQUIRED)
if(NOT TARGET_PROVIDER_FOUND OR NOT TARGET_PROVIDER_Native_FOUND
   OR NOT TARGET_PROVIDER_Catalog_FOUND OR NOT TARGET_PROVIDER_Objects_FOUND
   OR NOT TARGET_PROVIDER_Temporary_FOUND)
    message(FATAL_ERROR "The complete target provider dependency closure was not selected")
endif()
file(WRITE "${CMAKE_BINARY_DIR}/LVRSConfig.cmake" "set(LVRS_FOUND TRUE)\n")
install(FILES "${CMAKE_BINARY_DIR}/LVRSConfig.cmake" DESTINATION lib/cmake/LVRS)
]=])
execute_process(COMMAND "${CMAKE_COMMAND}"
    "-DLVRS_BOOTSTRAP_SOURCE_DIR=${_root}/child"
    "-DLVRS_BOOTSTRAP_BINARY_DIR=${_root}/build"
    "-DLVRS_BOOTSTRAP_INSTALL_PREFIX=${_root}/installed"
    "-DLVRS_BOOTSTRAP_FILE_PROVIDER_SOURCE_DIR=${_root}/provider"
    "-DLVRS_BOOTSTRAP_TOOLCHAIN_FILE=${_root}/target-toolchain.cmake"
    "-DLVRS_BOOTSTRAP_PREFIX_PATH=${_root}/host provider"
    -DLVRS_BOOTSTRAP_PLATFORM=wasm
    -DLVRS_BOOTSTRAP_ANDROID_ABI=forwarded-target-abi
    -P "${LVRS_SOURCE_DIR}/cmake/LVRSBootstrapFrameworkAction.cmake"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Target provider isolation failed:\n${output}\n${errors}")
endif()
if(NOT EXISTS "${_root}/installed/deps/iiFileProvider/lib/cmake/iiFileProvider/iiFileProviderConfig.cmake")
    message(FATAL_ERROR "The target dependency was not installed with LVRS")
endif()
file(WRITE "${_root}/provider/CMakeLists.txt" "message(FATAL_ERROR \"Intentional provider failure\")\n")
execute_process(COMMAND "${CMAKE_COMMAND}"
    "-DLVRS_BOOTSTRAP_SOURCE_DIR=${_root}/child"
    "-DLVRS_BOOTSTRAP_BINARY_DIR=${_root}/failed/build"
    "-DLVRS_BOOTSTRAP_FILE_PROVIDER_SOURCE_DIR=${_root}/provider"
    -DLVRS_BOOTSTRAP_PLATFORM=wasm
    -P "${LVRS_SOURCE_DIR}/cmake/LVRSBootstrapFrameworkAction.cmake"
    RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
if(result EQUAL 0 OR EXISTS "${_root}/failed/build/CMakeCache.txt")
    message(FATAL_ERROR "Dependency failure did not stop the LVRS child configure")
endif()
