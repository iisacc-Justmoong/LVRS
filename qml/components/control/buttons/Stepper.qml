import QtQuick
import LVRS 1.0

Item {
    id: control

    enum StepperArrow {
        UpDown,
        Up,
        Down
    }

    property int tone: AbstractButton.Primary
    property int arrow: Stepper.UpDown
    property bool enabled: true
    property int cornerRadius: Theme.radiusSm
    property color textColorDisabled: Theme.textOctonary

    signal clicked()
    signal pressed()
    signal released()
    signal canceled()

    readonly property bool effectiveEnabled: enabled && tone !== AbstractButton.Disabled
    readonly property bool hovered: interactionArea.containsMouse && effectiveEnabled
    readonly property bool down: interactionArea.pressed && effectiveEnabled
    readonly property real figmaStepperSize: Theme.iconSm
    readonly property real figmaChevronWidth: 10
    readonly property real figmaChevronHeight: 6
    readonly property real figmaUpDownChevronWidth: 6.43604
    readonly property real figmaUpDownChevronHeight: 11.1455
    readonly property real iconWidth: control.arrow === Stepper.UpDown ? figmaUpDownChevronWidth : figmaChevronWidth
    readonly property real iconHeight: control.arrow === Stepper.UpDown ? figmaUpDownChevronHeight : figmaChevronHeight
    readonly property rect iconBounds: Qt.rect(iconFrame.x, iconFrame.y, iconFrame.width, iconFrame.height)
    readonly property color backgroundColor: control.tone === AbstractButton.Borderless ? "transparent" : Theme.primary
    readonly property color backgroundColorHover: control.tone === AbstractButton.Borderless
        ? Theme.surfaceAlt
        : Qt.darker(Theme.primary, 1.12)
    readonly property color backgroundColorPressed: control.tone === AbstractButton.Borderless
        ? Theme.accentBlueMuted
        : Qt.darker(Theme.primary, 1.2)
    readonly property color backgroundColorDisabled: Theme.panelBackground04
    readonly property color resolvedBackgroundColor: !control.effectiveEnabled
        ? control.backgroundColorDisabled
        : control.down
            ? control.backgroundColorPressed
            : control.hovered
                ? control.backgroundColorHover
                : control.backgroundColor
    readonly property color resolvedIconColor: !control.effectiveEnabled
        ? control.textColorDisabled
        : control.tone === AbstractButton.Borderless
            ? Theme.primary
            : Theme.accentWhite

    function drawChevronDownPath(ctx) {
        ctx.beginPath()
        ctx.moveTo(5.00266, 6)
        ctx.bezierCurveTo(4.68401, 6, 4.39016, 5.869, 4.12108, 5.60699)
        ctx.lineTo(0.292087, 1.74498)
        ctx.bezierCurveTo(0.192954, 1.64367, 0.118605, 1.53537, 0.0690388, 1.42009)
        ctx.bezierCurveTo(0.0230129, 1.3048, 0, 1.17729, 0, 1.03755)
        ctx.bezierCurveTo(0, 0.848908, 0.0460258, 0.675983, 0.138078, 0.518777)
        ctx.bezierCurveTo(0.23367, 0.358079, 0.359356, 0.232314, 0.515135, 0.141485)
        ctx.bezierCurveTo(0.674456, 0.0471616, 0.847938, 0, 1.03558, 0)
        ctx.bezierCurveTo(1.3259, 0, 1.58435, 0.113537, 1.81094, 0.340611)
        ctx.lineTo(5.18853, 3.78865)
        ctx.lineTo(4.8274, 3.78865)
        ctx.lineTo(8.19437, 0.340611)
        ctx.bezierCurveTo(8.41388, 0.113537, 8.66879, 0, 8.95911, 0)
        ctx.bezierCurveTo(9.15029, 0, 9.32377, 0.0471616, 9.47955, 0.141485)
        ctx.bezierCurveTo(9.63887, 0.232314, 9.76456, 0.358079, 9.85661, 0.518777)
        ctx.bezierCurveTo(9.9522, 0.675983, 10, 0.848908, 10, 1.03755)
        ctx.bezierCurveTo(10, 1.31703, 9.90087, 1.55284, 9.7026, 1.74498)
        ctx.lineTo(5.87892, 5.60699)
        ctx.bezierCurveTo(5.74084, 5.74323, 5.60099, 5.84279, 5.45937, 5.90568)
        ctx.bezierCurveTo(5.3213, 5.96856, 5.16906, 6, 5.00266, 6)
        ctx.closePath()
    }

    function drawUpDownTopPath(ctx) {
        ctx.beginPath()
        ctx.moveTo(3.21973, 0)
        ctx.bezierCurveTo(3.01465, 0, 2.82552, 0.0854492, 2.65234, 0.256348)
        ctx.lineTo(0.187988, 2.77539)
        ctx.bezierCurveTo(0.124186, 2.84147, 0.0763346, 2.91211, 0.0444336, 2.9873)
        ctx.bezierCurveTo(0.0148112, 3.0625, 0, 3.14567, 0, 3.23682)
        ctx.bezierCurveTo(0, 3.35986, 0.0296224, 3.47266, 0.0888672, 3.5752)
        ctx.bezierCurveTo(0.150391, 3.68001, 0.231283, 3.76204, 0.331543, 3.82129)
        ctx.bezierCurveTo(0.434082, 3.88281, 0.545736, 3.91357, 0.666504, 3.91357)
        ctx.bezierCurveTo(0.853353, 3.91357, 1.01969, 3.83952, 1.16553, 3.69141)
        ctx.lineTo(3.33936, 1.44238)
        ctx.lineTo(3.10693, 1.44238)
        ctx.lineTo(5.27393, 3.69141)
        ctx.bezierCurveTo(5.4152, 3.83952, 5.57926, 3.91357, 5.76611, 3.91357)
        ctx.bezierCurveTo(5.88916, 3.91357, 6.00081, 3.88281, 6.10107, 3.82129)
        ctx.bezierCurveTo(6.20361, 3.76204, 6.28451, 3.68001, 6.34375, 3.5752)
        ctx.bezierCurveTo(6.40527, 3.47266, 6.43604, 3.35986, 6.43604, 3.23682)
        ctx.bezierCurveTo(6.43604, 3.05452, 6.37223, 2.90072, 6.24463, 2.77539)
        ctx.lineTo(3.78369, 0.256348)
        ctx.bezierCurveTo(3.69482, 0.16748, 3.60482, 0.102539, 3.51367, 0.0615234)
        ctx.bezierCurveTo(3.4248, 0.0205078, 3.32682, 0, 3.21973, 0)
        ctx.closePath()
    }

    function drawUpDownBottomPath(ctx) {
        ctx.beginPath()
        ctx.moveTo(3.21973, 11.1455)
        ctx.bezierCurveTo(3.01465, 11.1455, 2.82552, 11.0601, 2.65234, 10.8892)
        ctx.lineTo(0.187988, 8.37012)
        ctx.bezierCurveTo(0.124186, 8.30404, 0.0763346, 8.2334, 0.0444336, 8.1582)
        ctx.bezierCurveTo(0.0148112, 8.08301, 0, 7.99984, 0, 7.90869)
        ctx.bezierCurveTo(0, 7.78564, 0.0296224, 7.67285, 0.0888672, 7.57031)
        ctx.bezierCurveTo(0.150391, 7.46549, 0.231283, 7.38346, 0.331543, 7.32422)
        ctx.bezierCurveTo(0.434082, 7.2627, 0.545736, 7.23193, 0.666504, 7.23193)
        ctx.bezierCurveTo(0.853353, 7.23193, 1.01969, 7.30599, 1.16553, 7.4541)
        ctx.lineTo(3.33936, 9.70312)
        ctx.lineTo(3.10693, 9.70312)
        ctx.lineTo(5.27393, 7.4541)
        ctx.bezierCurveTo(5.4152, 7.30599, 5.57926, 7.23193, 5.76611, 7.23193)
        ctx.bezierCurveTo(5.88916, 7.23193, 6.00081, 7.2627, 6.10107, 7.32422)
        ctx.bezierCurveTo(6.20361, 7.38346, 6.28451, 7.46549, 6.34375, 7.57031)
        ctx.bezierCurveTo(6.40527, 7.67285, 6.43604, 7.78564, 6.43604, 7.90869)
        ctx.bezierCurveTo(6.43604, 8.09098, 6.37223, 8.24479, 6.24463, 8.37012)
        ctx.lineTo(3.78369, 10.8892)
        ctx.bezierCurveTo(3.69482, 10.978, 3.60482, 11.043, 3.51367, 11.084)
        ctx.bezierCurveTo(3.4248, 11.125, 3.32682, 11.1455, 3.21973, 11.1455)
        ctx.closePath()
    }

    implicitWidth: figmaStepperSize
    implicitHeight: figmaStepperSize
    width: figmaStepperSize
    height: figmaStepperSize
    clip: true

    Rectangle {
        anchors.fill: parent
        radius: control.cornerRadius
        antialiasing: true
        color: control.resolvedBackgroundColor
    }

    Item {
        id: iconFrame
        x: (control.width - width) * 0.5
        y: (control.height - height) * 0.5
        width: control.iconWidth
        height: control.iconHeight
        implicitWidth: width
        implicitHeight: height

        Canvas {
            id: iconCanvas
            anchors.fill: parent
            antialiasing: true

            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = control.resolvedIconColor

                if (control.arrow === Stepper.UpDown) {
                    ctx.save()
                    ctx.scale(width / control.figmaUpDownChevronWidth, height / control.figmaUpDownChevronHeight)
                    control.drawUpDownTopPath(ctx)
                    ctx.fill()
                    control.drawUpDownBottomPath(ctx)
                    ctx.fill()
                    ctx.restore()
                    return
                }

                ctx.save()
                ctx.scale(width / control.figmaChevronWidth, height / control.figmaChevronHeight)
                if (control.arrow === Stepper.Up) {
                    ctx.translate(control.figmaChevronWidth * 0.5, control.figmaChevronHeight * 0.5)
                    ctx.rotate(Math.PI)
                    ctx.translate(-control.figmaChevronWidth * 0.5, -control.figmaChevronHeight * 0.5)
                }
                control.drawChevronDownPath(ctx)
                ctx.fill()
                ctx.restore()
            }
        }
    }

    MouseArea {
        id: interactionArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        hoverEnabled: control.effectiveEnabled
        preventStealing: true
        enabled: control.effectiveEnabled

        onPressed: control.pressed()
        onReleased: control.released()
        onCanceled: control.canceled()
        onClicked: control.clicked()
    }

    onArrowChanged: iconCanvas.requestPaint()
    onResolvedIconColorChanged: iconCanvas.requestPaint()

    Component.onCompleted: iconCanvas.requestPaint()
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Stepper { tone: LV.AbstractButton.Primary; arrow: LV.Stepper.UpDown }
