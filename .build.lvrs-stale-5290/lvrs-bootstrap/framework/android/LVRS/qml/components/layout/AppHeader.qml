import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LVRS 1.0

ToolBar {
    id: root

    property string title: ""
    property string subtitle: ""
    property bool menuVisible: false
    property bool compact: false
    property int contentHorizontalPadding: compact ? Theme.gap10 : Theme.gap16
    property int contentVerticalPadding: compact ? Theme.gap10 : Theme.gap16
    property int rowSpacing: compact ? Theme.gap8 : Theme.gap12
    property int actionSpacing: compact ? Theme.gap6 : Theme.gap8
    property int menuButtonPadding: compact ? Theme.gap8 : Theme.gap10

    signal menuClicked()

    default property alias actions: actionRow.data

    implicitHeight: Math.max(Theme.headerMinHeight, contentRow.implicitHeight + Theme.headerExtraHeight)


    background: Rectangle {
        color: Theme.windowAlt
    }

    RowLayout {
        id: contentRow
        anchors.fill: parent
        anchors.leftMargin: root.contentHorizontalPadding
        anchors.rightMargin: root.contentHorizontalPadding
        anchors.topMargin: root.contentVerticalPadding
        anchors.bottomMargin: root.contentVerticalPadding
        spacing: root.rowSpacing

        ToolButton {
            id: menuButton
            visible: root.menuVisible
            text: "Menu"
            padding: root.menuButtonPadding

            contentItem: Label {
                style: description
                text: menuButton.text
                color: Theme.textPrimary
            }

            background: Rectangle {
                radius: Theme.radiusSm
                color: menuButton.down ? Theme.surfaceAlt : Theme.surfaceSolid
            }

            onClicked: {
                root.menuClicked()
            }
        }

        ColumnLayout {
            spacing: Theme.gap2
            Layout.fillWidth: true

            Label {
                style: title2
                text: root.title
                color: Theme.textPrimary
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                style: description
                visible: root.subtitle.length > 0
                text: root.subtitle
                color: Theme.textSecondary
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        RowLayout {
            id: actionRow
            spacing: root.actionSpacing
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.AppHeader { title: "Dashboard"; subtitle: "Overview" }
