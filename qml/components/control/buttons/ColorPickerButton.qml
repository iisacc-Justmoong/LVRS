import QtQuick
import LVRS 1.0

// Figma 892:62. The ring is exported from 892:4, including its angular gradient.
AbstractButton {
    id: control
    enum ButtonSize { Small = 22, Medium = 28, Large = 36 }
    property int buttonSize: ColorPickerButton.Medium
    property color currentColor: "#7A5AF8"
    property bool showHueRing: true
    readonly property real wellSize: Math.max(0, Math.min(availableWidth, availableHeight))

    implicitWidth: Theme.scaleMetric(buttonSize)
    implicitHeight: Theme.scaleMetric(buttonSize)
    horizontalPadding: Theme.gap4
    verticalPadding: Theme.gap4
    cornerRadius: Theme.radiusSm
    showFocusRing: false
    opacity: effectiveEnabled ? 1 : 0.32
    SpringBehavior on opacity { motionEnabled: control.motionEnabled; duration: Motion.colorDuration; easingType: Easing.OutCubic }
    backgroundColor: Theme.panelBackground06
    backgroundColorHover: Theme.panelBackground12
    backgroundColorPressed: Theme.panelBackground03
    backgroundColorDisabled: Theme.panelBackground06
    Accessible.name: qsTr("Choose color")
    Accessible.description: currentColor.toString()

    contentItem: Item {
        Image {
            objectName: "colorPickerButton_hueRing"
            anchors.centerIn: parent
            width: control.wellSize
            height: width
            visible: control.showHueRing
            source: "qrc:/qt/qml/LVRS/resources/images/color-picker-hue-ring.png"
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
        }
        Rectangle {
            objectName: "colorPickerButton_currentColor"
            anchors.centerIn: parent
            width: control.wellSize * (18 / 28)
            height: width
            radius: width / 2
            color: control.currentColor
            antialiasing: true
        }
    }
    background: Rectangle {
        objectName: "colorPickerButton_background"
        readonly property real borderWidth: control.effectiveEnabled && control.visualFocus
            ? Theme.scaleMetric(2) : Theme.scaleRealMetric(0.5)
        radius: control.resolvedCornerRadius
        color: !control.effectiveEnabled ? control.backgroundColorDisabled
            : control.down ? control.backgroundColorPressed
            : control.hovered ? control.backgroundColorHover : control.backgroundColor
        border.width: borderWidth
        border.color: control.effectiveEnabled && control.visualFocus
            ? Theme.primary : Qt.rgba(1, 1, 1, 0.2)
        antialiasing: true
        StateColorBehavior on color { motionEnabled: control.motionEnabled && control.effectiveEnabled }
        StateColorBehavior on border.color { motionEnabled: control.motionEnabled && control.effectiveEnabled }
    }
}
