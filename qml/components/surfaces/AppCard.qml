import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LVRS 1.0

Rectangle {
    id: root

    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1
    property int shapeStyle: shapeRoundRect
    property int cornerRadius: Theme.radiusLg
    property string title: ""
    property string subtitle: ""
    readonly property int cardPadding: Theme.gap18
    readonly property int sectionSpacing: Theme.gap10
    readonly property real resolvedCornerRadius: shapeStyle === shapeCylinder
        ? Math.max(0, Math.min(width, height) / 2)
        : cornerRadius

    default property alias content: contentSlot.data

    implicitWidth: Math.max(
                       Theme.dialogMinWidth,
                       cardPadding * 2 + Math.max(headerBlock.implicitWidth, contentSlot.childrenRect.width)
                   )
    implicitHeight: cardPadding * 2
                    + headerBlock.implicitHeight
                    + sectionSpacing
                    + separator.height
                    + sectionSpacing
                    + Math.max(1, contentSlot.childrenRect.height)
    clip: true

    radius: resolvedCornerRadius
    color: Theme.surfaceSolid

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.cardPadding
        spacing: root.sectionSpacing

        ColumnLayout {
            id: headerBlock
            spacing: Theme.gap4
            Layout.fillWidth: true
            Layout.fillHeight: false
            Layout.preferredHeight: implicitHeight

            Label {
                style: header
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

        Rectangle {
            id: separator
            Layout.fillWidth: true
            Layout.fillHeight: false
            Layout.preferredHeight: Theme.strokeThin
            radius: Theme.strokeThin
            color: Theme.surfaceSolid
        }

        Item {
            id: contentSlot
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: Math.max(1, childrenRect.height)
            Layout.preferredHeight: Math.max(1, childrenRect.height)
            implicitHeight: Math.max(1, childrenRect.height)
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.AppCard { title: "Summary"; subtitle: "Detail" }
