import QtQuick
import LVRS 1.0

Item {
    id: control

    property string text: "Text"
    property int cellHeight: 24
    property int contentSpacing: Theme.gap8
    property color dividerColor: Theme.surface
    property color textColor: Theme.bodyColor
    property bool showDivider: true
    property bool clipContent: true

    implicitWidth: 234
    implicitHeight: cellHeight
    clip: clipContent

    Row {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: control.contentSpacing

        Rectangle {
            visible: control.showDivider
            width: Theme.strokeThin
            height: control.cellHeight
            color: control.dividerColor
            antialiasing: false
        }

        Label {
            style: body
            text: control.text
            color: control.textColor
            elide: Text.ElideRight
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.TableCellItem { text: "Text" }
