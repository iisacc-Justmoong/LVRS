import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    checkable: true
    tone: AbstractButton.Borderless

    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1
    property int shapeStyle: shapeRoundRect
    property int boxSize: Theme.scaleMetric(17)
    property real boxRadius: Theme.scaleRealMetric(3.5)
    property color checkColor: Theme.bodyColor
    property color checkedColor: Theme.accent
    property color uncheckedColor: Theme.bodyColor
    property color disabledCheckedColor: Theme.panelBackground12
    property color disabledUncheckedColor: Theme.panelBackground12
    property color checkMarkColorDisabled: Theme.disabledColor
    property int checkMarkStrokeWidth: Theme.gap2
    property real boxBorderWidthCheckedEnabled: 0
    property real boxBorderWidthCheckedDisabled: Theme.scaleRealMetric(0.5)
    property real boxBorderWidthUncheckedEnabled: Theme.scaleRealMetric(0.5)
    property real boxBorderWidthUncheckedDisabled: 0
    property color boxBorderColorCheckedEnabled: "transparent"
    property color boxBorderColorCheckedDisabled: Theme.panelBackground12
    property color boxBorderColorUncheckedEnabled: Theme.bodyColor
    property color boxBorderColorUncheckedDisabled: "transparent"
    property color innerShadowSoftColor: "#14000000"
    property color innerShadowStrongColor: "#1A000000"

    readonly property bool showInnerShadow: !(control.checked && control.enabled)
    readonly property color resolvedCheckedFillColor: !control.enabled
        ? control.disabledCheckedColor
        : control.down
            ? Qt.darker(control.checkedColor, 1.2)
            : control.hovered
                ? Qt.darker(control.checkedColor, 1.08)
                : control.checkedColor
    readonly property color resolvedUncheckedFillColor: !control.enabled
        ? control.disabledUncheckedColor
        : control.down
            ? Qt.darker(control.uncheckedColor, 1.18)
            : control.hovered
                ? Qt.darker(control.uncheckedColor, 1.06)
                : control.uncheckedColor
    readonly property real resolvedBoxRadius: shapeStyle === shapeCylinder
        ? Math.max(0, boxSize / 2)
        : boxRadius
    readonly property real resolvedBoxBorderWidth: control.checked
        ? (control.enabled ? control.boxBorderWidthCheckedEnabled : control.boxBorderWidthCheckedDisabled)
        : (control.enabled ? control.boxBorderWidthUncheckedEnabled : control.boxBorderWidthUncheckedDisabled)
    readonly property color resolvedBoxBorderColor: control.checked
        ? (control.enabled ? control.boxBorderColorCheckedEnabled : control.boxBorderColorCheckedDisabled)
        : (control.enabled ? control.boxBorderColorUncheckedEnabled : control.boxBorderColorUncheckedDisabled)

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0
    spacing: Theme.gapNone
    backgroundColor: "transparent"
    backgroundColorHover: "transparent"
    backgroundColorPressed: "transparent"
    backgroundColorDisabled: "transparent"
    implicitHeight: contentItem.implicitHeight
    implicitWidth: contentItem.implicitWidth

    background: Item { }

    contentItem: RowLayout {
        spacing: control.text.length > 0 ? Theme.gap6 : Theme.gapNone
        Layout.alignment: Qt.AlignVCenter

        Rectangle {
            implicitWidth: control.boxSize
            implicitHeight: control.boxSize
            Layout.preferredWidth: control.boxSize
            Layout.preferredHeight: control.boxSize
            radius: control.resolvedBoxRadius
            color: control.checked ? control.resolvedCheckedFillColor : control.resolvedUncheckedFillColor
            border.width: control.resolvedBoxBorderWidth
            border.color: control.resolvedBoxBorderColor
            antialiasing: true

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: "transparent"
                border.width: Theme.scaleRealMetric(1)
                border.color: control.innerShadowSoftColor
                visible: control.showInnerShadow
                antialiasing: true
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: parent.height * 0.45
                radius: parent.radius
                color: control.innerShadowStrongColor
                opacity: control.showInnerShadow ? 0.5 : 0
                antialiasing: true
                clip: true
            }

            Canvas {
                id: checkmarkCanvas
                anchors.fill: parent
                visible: control.checked
                antialiasing: true

                onPaint: {
                    const ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    if (!control.checked)
                        return

                    ctx.beginPath()
                    ctx.moveTo(width * 0.25, height * 0.52)
                    ctx.lineTo(width * 0.43, height * 0.70)
                    ctx.lineTo(width * 0.69, height * 0.34)
                    ctx.lineWidth = control.checkMarkStrokeWidth
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    ctx.strokeStyle = control.enabled ? control.checkColor : control.checkMarkColorDisabled
                    ctx.stroke()
                }
            }
        }

        Label {
            style: body
            text: control.text
            color: control.enabled ? Theme.bodyColor : Theme.disabledColor
            visible: control.text.length > 0
            Layout.alignment: Qt.AlignVCenter
        }
    }

    onCheckedChanged: checkmarkCanvas.requestPaint()
    onEnabledChanged: checkmarkCanvas.requestPaint()
    onCheckColorChanged: checkmarkCanvas.requestPaint()
    onCheckMarkColorDisabledChanged: checkmarkCanvas.requestPaint()
    onCheckMarkStrokeWidthChanged: checkmarkCanvas.requestPaint()

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.CheckBox { text: "Remember me"; checked: true }
