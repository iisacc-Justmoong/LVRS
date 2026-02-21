#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /Users/ymy/Qt/6.8.3/macos/libexec/rcc --output /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.qt/rcc/qrc_LVRSCore_raw_qml_0.cpp --name LVRSCore_raw_qml_0 /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.qt/rcc/LVRSCore_raw_qml_0.qrc --no-zstd
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /Users/ymy/Qt/6.8.3/macos/libexec/rcc --output /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.qt/rcc/qrc_LVRSCore_raw_qml_0.cpp --name LVRSCore_raw_qml_0 /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.qt/rcc/LVRSCore_raw_qml_0.qrc --no-zstd
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /Users/ymy/Qt/6.8.3/macos/libexec/rcc --output /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.qt/rcc/qrc_LVRSCore_raw_qml_0.cpp --name LVRSCore_raw_qml_0 /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.qt/rcc/LVRSCore_raw_qml_0.qrc --no-zstd
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /Users/ymy/Qt/6.8.3/macos/libexec/rcc --output /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.qt/rcc/qrc_LVRSCore_raw_qml_0.cpp --name LVRSCore_raw_qml_0 /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.qt/rcc/LVRSCore_raw_qml_0.qrc --no-zstd
fi

