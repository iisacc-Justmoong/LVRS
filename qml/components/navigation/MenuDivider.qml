import QtQuick
import LVRS 1.0

Item {
    id: control

    // axis: "horizontal" | "vertical"
    property string axis: "horizontal"
    property color dividerColor: Theme.contextMenuDivider
    property real thickness: Theme.scaleRealMetric(0.2)
    property int crossPadding: Theme.scaleMetric(1)
    property int lineLength: Theme.scaleMetric(220)

    readonly property bool verticalAxis: {
        const normalized = axis === undefined || axis === null
            ? ""
            : String(axis).trim().toLowerCase()
        return normalized === "vertical"
    }

    implicitWidth: verticalAxis ? (thickness + (crossPadding * 2)) : lineLength
    implicitHeight: verticalAxis ? lineLength : (thickness + (crossPadding * 2))

    Rectangle {
        anchors.centerIn: parent
        width: control.verticalAxis ? control.thickness : parent.width
        height: control.verticalAxis ? parent.height : control.thickness
        color: control.dividerColor
        antialiasing: true
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.MenuDivider { axis: "horizontal" }
