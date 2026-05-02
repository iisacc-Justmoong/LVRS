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

    readonly property bool usingStateModel: stateModel !== undefined && stateModel !== null
    readonly property int stateRevision: usingStateModel && stateModel.revision !== undefined ? stateModel.revision : 0
    readonly property real effectiveMinimumValue: stateNumber(minimumValueStateKey, minimumValue)
    readonly property real effectiveMaximumValue: stateNumber(maximumValueStateKey, maximumValue)
    readonly property real effectiveStartValue: stateNumber(startValueStateKey, startValue)
    readonly property real effectiveCurrentValue: stateNumber(currentValueStateKey, currentValue)
    readonly property real barHeight: size === regular ? regularHeight : largeHeight
    readonly property real valueRange: effectiveMaximumValue - effectiveMinimumValue
    readonly property real normalizedStart: normalizedValue(effectiveStartValue)
    readonly property real normalizedCurrent: normalizedValue(effectiveCurrentValue)
    readonly property real fillStart: Math.min(normalizedStart, normalizedCurrent)
    readonly property real fillProgress: Math.abs(normalizedCurrent - normalizedStart)
    readonly property real progress: normalizedCurrent

    function stateNumber(key, fallbackValue) {
        stateRevision
        const fallbackNumber = Number(fallbackValue)
        const model = control.stateModel
        if (!model || model.value === undefined)
            return Number.isFinite(fallbackNumber) ? fallbackNumber : 0
        const rawValue = model.valueOr !== undefined
            ? model.valueOr(key, fallbackNumber)
            : model.value(key, fallbackNumber)
        const nextNumber = Number(rawValue)
        return Number.isFinite(nextNumber)
            ? nextNumber
            : Number.isFinite(fallbackNumber)
                ? fallbackNumber
                : 0
    }

    function resolvedRadius(rectWidth, rectHeight) {
        if (shapeStyle === shapeCylinder)
            return Math.max(0, Math.min(rectWidth, rectHeight) / 2)
        return cornerRadius
    }

    function normalizedValue(value) {
        const range = control.valueRange
        if (Math.abs(range) < 0.000001)
            return Number(value) >= control.effectiveMaximumValue ? 1 : 0
        const normalized = (Number(value) - control.effectiveMinimumValue) / range
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
// LV.ProgressBar {
//     stateModel: progressState
// }
