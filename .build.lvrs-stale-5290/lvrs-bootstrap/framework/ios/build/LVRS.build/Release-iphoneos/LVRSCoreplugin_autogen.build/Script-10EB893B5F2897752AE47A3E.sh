#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /opt/homebrew/bin/cmake -E cmake_autogen /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/CMakeFiles/LVRSCoreplugin_autogen.dir/AutogenInfo.json Debug
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /opt/homebrew/bin/cmake -E cmake_autogen /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/CMakeFiles/LVRSCoreplugin_autogen.dir/AutogenInfo.json Release
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /opt/homebrew/bin/cmake -E cmake_autogen /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/CMakeFiles/LVRSCoreplugin_autogen.dir/AutogenInfo.json MinSizeRel
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /opt/homebrew/bin/cmake -E cmake_autogen /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/CMakeFiles/LVRSCoreplugin_autogen.dir/AutogenInfo.json RelWithDebInfo
fi

