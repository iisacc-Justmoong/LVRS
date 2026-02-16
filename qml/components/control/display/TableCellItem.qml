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

    Rectangle {
        id: dividerNode
        visible: control.showDivider
        width: Theme.strokeThin
        height: parent.height
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        color: control.dividerColor
        antialiasing: false
    }

    Label {
        anchors.left: dividerNode.visible ? dividerNode.right : parent.left
        anchors.leftMargin: dividerNode.visible ? control.contentSpacing : 0
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        style: body
        text: control.text
        color: control.textColor
        elide: Text.ElideRight
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.TableCellItem { text: "Text" }
