import QtQuick
import LVRS 1.0

Item {
    id: control

    property int horizontalPadding: Theme.gap4
    property int verticalPadding: Theme.gap4
    property int spacing: Theme.gap2
    property int borderWidth: 2
    property int cornerRadius: Theme.radiusMd
    property color backgroundColor: Theme.panelBackground08
    property color borderColor: Theme.panelBackground12
    property bool forceBorderlessTone: true
    property bool _syncScheduled: false

    readonly property int segmentCount: collectSegmentButtons().length
    default property alias buttons: segmentRow.data

    function collectSegmentButtons() {
        const result = []
        for (let i = 0; i < segmentRow.children.length; i++) {
            const child = segmentRow.children[i]
            if (!child || child.tone === undefined)
                continue
            result.push(child)
        }
        return result
    }

    function applySegmentStyle(button) {
        if (!button)
            return
        if (control.forceBorderlessTone && button.tone !== undefined)
            button.tone = AbstractButton.Borderless
    }

    function syncSegmentStyles() {
        const segments = collectSegmentButtons()
        for (let i = 0; i < segments.length; i++)
            applySegmentStyle(segments[i])
    }

    function scheduleSyncSegmentStyles() {
        if (_syncScheduled)
            return
        _syncScheduled = true
        Qt.callLater(function() {
            _syncScheduled = false
            syncSegmentStyles()
        })
    }

    implicitWidth: segmentRow.implicitWidth + (horizontalPadding * 2)
    implicitHeight: segmentRow.implicitHeight + (verticalPadding * 2)

    Rectangle {
        anchors.fill: parent
        color: control.backgroundColor
        radius: control.cornerRadius
        border.width: control.borderWidth
        border.color: control.borderColor
        antialiasing: true
    }

    Row {
        id: segmentRow
        anchors.fill: parent
        anchors.leftMargin: control.horizontalPadding
        anchors.rightMargin: control.horizontalPadding
        anchors.topMargin: control.verticalPadding
        anchors.bottomMargin: control.verticalPadding
        spacing: control.spacing
    }

    onForceBorderlessToneChanged: scheduleSyncSegmentStyles()

    Connections {
        target: segmentRow
        function onChildrenChanged() {
            control.scheduleSyncSegmentStyles()
        }
    }

    QtObject {
        Component.onCompleted: {
            control.syncSegmentStyles()
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.LabelSegmentedControl {
//     LV.LabelButton { text: "Button" }
//     LV.LabelButton { text: "Button" }
// }
