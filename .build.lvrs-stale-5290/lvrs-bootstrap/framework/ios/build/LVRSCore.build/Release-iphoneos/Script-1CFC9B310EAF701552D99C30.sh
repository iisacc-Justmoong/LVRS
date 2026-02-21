#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /Users/ymy/Qt/6.8.3/macos/libexec/qmlcachegen --resource-name qmlcache_LVRSCore -o /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.rcc/qmlcache/LVRSCore_qmlcache_loader.cpp @/Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.rcc/qmlcache/LVRSCore_qml_loader_file_list.rsp
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /Users/ymy/Qt/6.8.3/macos/libexec/qmlcachegen --resource-name qmlcache_LVRSCore -o /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.rcc/qmlcache/LVRSCore_qmlcache_loader.cpp @/Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.rcc/qmlcache/LVRSCore_qml_loader_file_list.rsp
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /Users/ymy/Qt/6.8.3/macos/libexec/qmlcachegen --resource-name qmlcache_LVRSCore -o /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.rcc/qmlcache/LVRSCore_qmlcache_loader.cpp @/Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.rcc/qmlcache/LVRSCore_qml_loader_file_list.rsp
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /Users/ymy/Qt/6.8.3/macos/libexec/qmlcachegen --resource-name qmlcache_LVRSCore -o /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.rcc/qmlcache/LVRSCore_qmlcache_loader.cpp @/Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/.rcc/qmlcache/LVRSCore_qml_loader_file_list.rsp
fi

