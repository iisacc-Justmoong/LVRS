import QtQuick
import LVRS 1.0

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
    property var stateModel: null
    property string minimumValueStateKey: "minimumValue"
    property string maximumValueStateKey: "maximumValue"
    property string startValueStateKey: "startValue"
    property string currentValueStateKey: "currentValue"

    property color trackColor: "#0D000000"
    property color fillColor: "#007AFF"
    property real cornerRadius: Theme.scaleRealMetric(100)
    property real largeHeight: Theme.scaleRealMetric(6)
    property real regularHeight: Theme.scaleRealMetric(3)

    ProgressModel {
        id: progressModel
        minimumValue: control.minimumValue
        maximumValue: control.maximumValue
        startValue: control.startValue
        currentValue: control.currentValue
        stateModel: control.stateModel
        minimumValueStateKey: control.minimumValueStateKey
        maximumValueStateKey: control.maximumValueStateKey
        startValueStateKey: control.startValueStateKey
        currentValueStateKey: control.currentValueStateKey
    }

    readonly property bool usingStateModel: progressModel.usingStateModel
    readonly property int stateRevision: progressModel.stateRevision
    readonly property real effectiveMinimumValue: progressModel.effectiveMinimumValue
    readonly property real effectiveMaximumValue: progressModel.effectiveMaximumValue
    readonly property real effectiveStartValue: progressModel.effectiveStartValue
    readonly property real effectiveCurrentValue: progressModel.effectiveCurrentValue
    readonly property real barHeight: size === regular ? regularHeight : largeHeight
    readonly property real valueRange: progressModel.valueRange
    readonly property real normalizedStart: progressModel.normalizedStart
    readonly property real normalizedCurrent: progressModel.normalizedCurrent
    readonly property real fillStart: progressModel.fillStart
    readonly property real fillProgress: progressModel.fillProgress
    readonly property real progress: progressModel.progress

    function stateNumber(key, fallbackValue) {
        return progressModel.stateNumber(key, fallbackValue)
    }

    function resolvedRadius(rectWidth, rectHeight) {
        return progressModel.radiusFor(shapeStyle, cornerRadius, rectWidth, rectHeight)
    }

    function normalizedValue(value) {
        return progressModel.normalizedValue(value)
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
// LV.ProgressBar {
//     stateModel: progressState
// }
