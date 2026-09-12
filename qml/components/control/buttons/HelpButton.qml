import QtQuick
import LVRS 1.0

// Figma 44:819: the question mark is text, not an icon.
AbstractButton {
    id: control
    text: "?"
    implicitWidth: Theme.scaleMetric(21)
    implicitHeight: Theme.scaleMetric(21)
    horizontalPadding: Theme.gap4
    verticalPadding: Theme.gap2
    spacing: Theme.gap10
    cornerRadius: Theme.scaleMetric(100)
    backgroundColor: Theme.panelBackground04
    textColor: Theme.descriptionColor
    Accessible.name: qsTr("Help")

    contentItem: Item {
        Label {
            objectName: "helpButton_label"
            anchors.centerIn: parent
            anchors.verticalCenterOffset: Theme.scaleRealMetric(0.5)
            text: control.text
            style: description
            color: control.effectiveEnabled ? control.textColor : control.textColorDisabled
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
