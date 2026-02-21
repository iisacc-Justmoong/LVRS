#!/bin/sh
set -e
if test "$CONFIGURATION" = "Debug"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /Users/ymy/Qt/6.8.3/macos/libexec/moc -o /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/qt6lvrscoreplugin_init_metatypes.json.gen --collect-json @/Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/LVRSCoreplugin_init_json_file_list.txt
  /opt/homebrew/bin/cmake -E copy_if_different /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/qt6lvrscoreplugin_init_metatypes.json.gen /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/qt6lvrscoreplugin_init_metatypes.json
fi
if test "$CONFIGURATION" = "Release"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /Users/ymy/Qt/6.8.3/macos/libexec/moc -o /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/qt6lvrscoreplugin_init_metatypes.json.gen --collect-json @/Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/LVRSCoreplugin_init_json_file_list.txt
  /opt/homebrew/bin/cmake -E copy_if_different /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/qt6lvrscoreplugin_init_metatypes.json.gen /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/qt6lvrscoreplugin_init_metatypes.json
fi
if test "$CONFIGURATION" = "MinSizeRel"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /Users/ymy/Qt/6.8.3/macos/libexec/moc -o /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/qt6lvrscoreplugin_init_metatypes.json.gen --collect-json @/Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/LVRSCoreplugin_init_json_file_list.txt
  /opt/homebrew/bin/cmake -E copy_if_different /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/qt6lvrscoreplugin_init_metatypes.json.gen /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/qt6lvrscoreplugin_init_metatypes.json
fi
if test "$CONFIGURATION" = "RelWithDebInfo"; then :
  cd /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios
  /Users/ymy/Qt/6.8.3/macos/libexec/moc -o /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/qt6lvrscoreplugin_init_metatypes.json.gen --collect-json @/Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/LVRSCoreplugin_init_json_file_list.txt
  /opt/homebrew/bin/cmake -E copy_if_different /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/qt6lvrscoreplugin_init_metatypes.json.gen /Volumes/Storage/static/InfraSystem/LVRS/build/lvrs-bootstrap/framework/ios/meta_types/qt6lvrscoreplugin_init_metatypes.json
fi

