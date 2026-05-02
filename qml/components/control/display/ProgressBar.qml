import QtQuick

Item {
    id: control

    // Size constants for API usage: LV.ProgressBar { size: regular }
    readonly property int large: 0
    readonly property int regular: 1
    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1

    property int size: large
    property int shapeStyle: shapeRoundRect
    property real minimumValue: 0
    property real maximumValue: 100
    property real startValue: 0
    property real currentValue: 0
    property alias endValue: control.maximumValue

    property color trackColor: "#0D000000"
    property color fillColor: "#007AFF"
    property real cornerRadius: Theme.scaleRealMetric(100)
    property real largeHeight: Theme.scaleRealMetric(6)
    property real regularHeight: Theme.scaleRealMetric(3)

    readonly property real barHeight: size === regular ? regularHeight : largeHeight
    readonly property real valueRange: maximumValue - minimumValue
    readonly property real normalizedStart: normalizedValue(startValue)
    readonly property real normalizedCurrent: normalizedValue(currentValue)
    readonly property real fillStart: Math.min(normalizedStart, normalizedCurrent)
    readonly property real fillProgress: Math.abs(normalizedCurrent - normalizedStart)
    readonly property real progress: normalizedCurrent

    function resolvedRadius(rectWidth, rectHeight) {
        if (shapeStyle === shapeCylinder)
            return Math.max(0, Math.min(rectWidth, rectHeight) / 2)
        return cornerRadius
    }

    function normalizedValue(value) {
        const range = control.valueRange
        if (Math.abs(range) < 0.000001)
            return Number(value) >= control.maximumValue ? 1 : 0
        const normalized = (Number(value) - control.minimumValue) / range
        if (isNaN(normalized))
            return 0
        return Math.max(0, Math.min(1, normalized))
    }

    implicitWidth: Theme.scaleMetric(100)
    implicitHeight: barHeight

    Rectangle {
        id: track
        anchors.fill: parent
        radius: control.resolvedRadius(width, height)
        color: control.trackColor
        antialiasing: true
    }

    Rectangle {
        id: fill
        x: track.width * control.fillStart
        y: 0
        width: track.width * control.fillProgress
        height: track.height
        radius: control.resolvedRadius(width, height)
        color: control.fillColor
        antialiasing: true
        visible: width > 0
    }

    Rectangle {
        anchors.fill: parent
        radius: control.resolvedRadius(width, height)
        color: "transparent"
        border.width: Theme.scaleRealMetric(1)
        border.color: "#14000000"
        antialiasing: true
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ProgressBar {
//     width: 180
//     size: regular
//     minimumValue: 0
//     maximumValue: 100
//     startValue: 0
//     currentValue: 64
// }
