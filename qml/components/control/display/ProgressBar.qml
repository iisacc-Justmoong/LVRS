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
    property real startValue: 0
    property real endValue: 100
    property real currentValue: 0

    property color trackColor: "#0D000000"
    property color fillColor: "#007AFF"
    property real cornerRadius: Theme.scaleRealMetric(100)
    property real largeHeight: Theme.scaleRealMetric(6)
    property real regularHeight: Theme.scaleRealMetric(3)

    readonly property real barHeight: size === regular ? regularHeight : largeHeight
    readonly property real valueRange: endValue - startValue
    readonly property real progress: {
        if (Math.abs(valueRange) < 0.000001)
            return currentValue >= endValue ? 1 : 0
        return Math.max(0, Math.min(1, (currentValue - startValue) / valueRange))
    }

    function resolvedRadius(rectWidth, rectHeight) {
        if (shapeStyle === shapeCylinder)
            return Math.max(0, Math.min(rectWidth, rectHeight) / 2)
        return cornerRadius
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
        x: 0
        y: 0
        width: track.width * control.progress
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
//     startValue: 0
//     endValue: 100
//     currentValue: 64
// }
