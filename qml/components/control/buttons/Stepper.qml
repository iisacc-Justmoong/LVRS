import QtQuick
import LVRS 1.0

AbstractButton {
    id: control

    enum StepperArrow {
        UpDown,
        Up,
        Down
    }

    tone: AbstractButton.Primary
    property int arrow: Stepper.UpDown

    readonly property real figmaStepperSize: Theme.iconSm
    readonly property real iconWidth: control.arrow === Stepper.UpDown ? 6.436 : 10
    readonly property real iconHeight: control.arrow === Stepper.UpDown ? 11.146 : 6
    readonly property real iconStrokeWidth: 1.25
    readonly property color resolvedIconColor: !control.effectiveEnabled
        ? control.textColorDisabled
        : control.tone === AbstractButton.Borderless
            ? Theme.primary
            : Theme.accentWhite

    horizontalPadding: Theme.gapNone
    verticalPadding: Theme.gapNone
    spacing: Theme.gapNone
    cornerRadius: Theme.radiusSm

    width: figmaStepperSize
    height: figmaStepperSize
    implicitWidth: figmaStepperSize
    implicitHeight: figmaStepperSize

    backgroundColor: control.tone === AbstractButton.Borderless ? "transparent" : Theme.primary
    backgroundColorHover: control.tone === AbstractButton.Borderless
        ? Theme.surfaceAlt
        : Qt.darker(Theme.primary, 1.12)
    backgroundColorPressed: control.tone === AbstractButton.Borderless
        ? Theme.accentBlueMuted
        : Qt.darker(Theme.primary, 1.2)

    contentItem: Item {
        implicitWidth: control.iconWidth
        implicitHeight: control.iconHeight
        width: implicitWidth
        height: implicitHeight

        Canvas {
            id: iconCanvas
            anchors.fill: parent
            antialiasing: true

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                ctx.beginPath()
                ctx.strokeStyle = control.resolvedIconColor
                ctx.lineWidth = control.iconStrokeWidth
                ctx.lineCap = "round"
                ctx.lineJoin = "round"

                if (control.arrow === Stepper.Up) {
                    ctx.moveTo(0.75, height - 0.75)
                    ctx.lineTo(width * 0.5, 0.75)
                    ctx.lineTo(width - 0.75, height - 0.75)
                } else if (control.arrow === Stepper.Down) {
                    ctx.moveTo(width - 0.75, 0.75)
                    ctx.lineTo(width * 0.5, height - 0.75)
                    ctx.lineTo(0.75, 0.75)
                } else {
                    const topBaseY = height * 0.5 - 0.9
                    const bottomBaseY = height * 0.5 + 0.9

                    ctx.moveTo(0.6, topBaseY)
                    ctx.lineTo(width * 0.5, 0.75)
                    ctx.lineTo(width - 0.6, topBaseY)

                    ctx.moveTo(width - 0.6, bottomBaseY)
                    ctx.lineTo(width * 0.5, height - 0.75)
                    ctx.lineTo(0.6, bottomBaseY)
                }

                ctx.stroke()
            }
        }
    }

    onArrowChanged: iconCanvas.requestPaint()
    onResolvedIconColorChanged: iconCanvas.requestPaint()
    onIconStrokeWidthChanged: iconCanvas.requestPaint()

    Component.onCompleted: iconCanvas.requestPaint()
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Stepper { tone: LV.AbstractButton.Primary; arrow: LV.Stepper.UpDown }
