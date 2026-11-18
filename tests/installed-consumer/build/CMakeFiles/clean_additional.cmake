# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles/lvrs_installed_avatar_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/lvrs_installed_avatar_autogen.dir/ParseCache.txt"
  "lvrs_installed_avatar_autogen"
  )
endif()
