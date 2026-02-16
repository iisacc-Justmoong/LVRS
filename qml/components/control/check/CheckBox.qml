import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    checkable: true
    tone: AbstractButton.Borderless

    property int boxSize: Theme.controlIndicatorSize
    property color checkColor: Theme.textPrimary
    property color checkedColor: Theme.accent
    property color uncheckedColor: Theme.surfaceAlt
    property color disabledCheckedColor: Theme.surfaceAlt
    property color disabledUncheckedColor: Theme.subSurface
    property color checkMarkColorDisabled: Theme.textTertiary
    property int checkMarkStrokeWidth: Theme.gap2

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
        spacing: control.text.length > 0 ? Theme.gap8 : Theme.gapNone
        Layout.alignment: Qt.AlignVCenter

        Rectangle {
            implicitWidth: control.boxSize
            implicitHeight: control.boxSize
            Layout.preferredWidth: control.boxSize
            Layout.preferredHeight: control.boxSize
            radius: Theme.radiusSm
            color: control.enabled
                ? (control.checked ? control.checkedColor : control.uncheckedColor)
                : (control.checked ? control.disabledCheckedColor : control.disabledUncheckedColor)
            antialiasing: true

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
                    ctx.moveTo(width * 0.22, height * 0.55)
                    ctx.lineTo(width * 0.42, height * 0.74)
                    ctx.lineTo(width * 0.78, height * 0.30)
                    ctx.lineWidth = control.checkMarkStrokeWidth
                    ctx.lineCap = "round"
                    ctx.lineJoin = "round"
                    ctx.strokeStyle = control.enabled ? control.checkColor : control.checkMarkColorDisabled
                    ctx.stroke()
                }
            }
        }

        Label {
            style: description
            text: control.text
            color: control.enabled ? Theme.textPrimary : Theme.textTertiary
            visible: control.text.length > 0
            Layout.alignment: Qt.AlignVCenter
        }
    }

    onCheckedChanged: checkmarkCanvas.requestPaint()
    onEnabledChanged: checkmarkCanvas.requestPaint()
    onCheckColorChanged: checkmarkCanvas.requestPaint()
    onCheckMarkColorDisabledChanged: checkmarkCanvas.requestPaint()

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.CheckBox { text: "Remember me"; checked: true }
