#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /opt/homebrew/bin/cmake -DFILES_INFO_PATH=/Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.qt/LVRSCore_qml.cmake -P /Users/ymy/Qt/6.8.3/ios/lib/cmake/Qt6Qml/Qt6QmlCopyFiles.cmake
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /opt/homebrew/bin/cmake -DFILES_INFO_PATH=/Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.qt/LVRSCore_qml.cmake -P /Users/ymy/Qt/6.8.3/ios/lib/cmake/Qt6Qml/Qt6QmlCopyFiles.cmake
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /opt/homebrew/bin/cmake -DFILES_INFO_PATH=/Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.qt/LVRSCore_qml.cmake -P /Users/ymy/Qt/6.8.3/ios/lib/cmake/Qt6Qml/Qt6QmlCopyFiles.cmake
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /opt/homebrew/bin/cmake -DFILES_INFO_PATH=/Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.qt/LVRSCore_qml.cmake -P /Users/ymy/Qt/6.8.3/ios/lib/cmake/Qt6Qml/Qt6QmlCopyFiles.cmake
fi

