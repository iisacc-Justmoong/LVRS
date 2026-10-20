cmake_minimum_required(VERSION 3.21)
include("${LVRS_SOURCE_DIR}/cmake/LVRSBootstrapAndroidNdk.cmake")
set(_root "${LVRS_BINARY_DIR}/tests/cpp23-ndk")
file(MAKE_DIRECTORY "${_root}/old" "${_root}/new/toolchains/llvm/prebuilt/test/sysroot/usr/include/c++/v1")
foreach(_header IN ITEMS stop_token expected)
    file(WRITE "${_root}/new/toolchains/llvm/prebuilt/test/sysroot/usr/include/c++/v1/${_header}" "// fixture\n")
endforeach()
set(ENV{CMAKE_ANDROID_NDK} "${_root}/old")
set(ENV{ANDROID_NDK_ROOT} "${_root}/new")
_lvrs_bootstrap_detect_android_ndk("${_root}/sdk" actual)
if(NOT actual STREQUAL "${_root}/new")
    message(FATAL_ERROR "An obsolete ambient NDK hid the compatible NDK")
endif()
file(WRITE "${_root}/explicit.cmake"
    "set(LVRS_BOOTSTRAP_ANDROID_NDK [==[${_root}/old]==])\ninclude([==[${LVRS_SOURCE_DIR}/cmake/LVRSBootstrapAndroidNdk.cmake]==])\n_lvrs_bootstrap_detect_android_ndk(\"\" actual)\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -P "${_root}/explicit.cmake"
    RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
if(result EQUAL 0)
    message(FATAL_ERROR "An explicitly unsupported NDK was silently accepted")
endif()

# Exercise the real parent helper: an obsolete shell hint must not be promoted
# into an explicit child override that disables the compatible ambient fallback.
file(MAKE_DIRECTORY "${_root}/parent" "${_root}/helpers" "${_root}/qt/lib/cmake/Qt6")
file(COPY "${LVRS_SOURCE_DIR}/cmake/LVRSHelpers.cmake"
    "${LVRS_SOURCE_DIR}/cmake/LVRSBootstrapAndroidNdk.cmake" DESTINATION "${_root}/helpers")
file(WRITE "${_root}/qt/lib/cmake/Qt6/Qt6Config.cmake" "set(Qt6_FOUND TRUE)\n")
file(WRITE "${_root}/helpers/LVRSBootstrapFrameworkAction.cmake" [=[
include("${CMAKE_CURRENT_LIST_DIR}/LVRSBootstrapAndroidNdk.cmake")
_lvrs_bootstrap_detect_android_ndk("" actual)
file(MAKE_DIRECTORY "${LVRS_BOOTSTRAP_BINARY_DIR}")
file(WRITE "${LVRS_BOOTSTRAP_BINARY_DIR}/selected-ndk.txt" "${actual}")
]=])
file(WRITE "${_root}/parent/CMakeLists.txt"
    "cmake_minimum_required(VERSION 3.21)\nproject(NdkParent LANGUAGES NONE)\nset(LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS android)\nset(LVRS_BOOTSTRAP_QT_PREFIX_ANDROID [==[${_root}/qt]==])\ninclude([==[${_root}/helpers/LVRSHelpers.cmake]==])\nlvrs_create_framework_bootstrap_targets()\n")
set(ENV{LVRS_BOOTSTRAP_ANDROID_NDK} "${_root}/old")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${_root}/parent" -B "${_root}/parent/build"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
if(result EQUAL 0)
    execute_process(COMMAND "${CMAKE_COMMAND}" --build "${_root}/parent/build" --target bootstrap_lvrs_android
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors)
endif()
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Obsolete environment hint blocked parent bootstrap: ${output}\n${errors}")
endif()
file(READ "${_root}/parent/build/lvrs-bootstrap/framework/android/selected-ndk.txt" actual)
if(NOT actual STREQUAL "${_root}/new")
    message(FATAL_ERROR "Parent bootstrap did not retain the compatible NDK fallback: ${actual}")
endif()
