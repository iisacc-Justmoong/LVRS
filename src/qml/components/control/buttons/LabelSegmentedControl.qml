import QtQuick
import LVRS 1.0

Item {
    id: control
    property bool motionEnabled: true

    readonly property int shapeRoundRect: 0
    readonly property int shapeCylinder: 1

    property int shapeStyle: shapeRoundRect
    property int horizontalPadding: Theme.gap4
    property real verticalPadding: Theme.scaleRealMetric(3.5)
    property int spacing: Theme.gap2
    property int borderWidth: Theme.scaleMetric(2)
    property int cornerRadius: Theme.radiusLg
    property color backgroundColor: Theme.panelBackground08
    property color borderColor: Theme.panelBackground12
    property bool forceBorderlessTone: true
    property bool _syncScheduled: false
    property var _styledSegments: []
    property alias method: methodRegistry.method
    property alias methods: methodRegistry.methods
    readonly property alias hasInjectedMethods: methodRegistry.hasInjectedMethods
    readonly property real resolvedCornerRadius: shapeStyle === shapeCylinder
        ? Math.max(0, Math.min(width, height) / 2)
        : cornerRadius

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

    function syncSegmentStyles() {
        const segments = collectSegmentButtons()
        for (let i = 0; i < _styledSegments.length; i++) {
            const old = _styledSegments[i]
            if (old && segments.indexOf(old) < 0 && old._segmentStyleOwner === control)
                old._segmentStyleOwner = null
        }
        for (let i = 0; i < segments.length; i++) {
            const button = segments[i]
            if (button._segmentStyleOwner !== undefined)
                button._segmentStyleOwner = control
        }
        _styledSegments = segments
        segmentRow.forceLayout()
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

    function createMethodEvent(triggerName) {
        return methodRegistry.createEvent(triggerName)
    }

    function invokeMethod(candidate, eventData) {
        return methodRegistry.invokeMethod(candidate, eventData)
    }

    function invokeMethods(eventData) {
        return methodRegistry.invokeMethods(eventData)
    }

    implicitWidth: segmentRow.implicitWidth + (horizontalPadding * 2)
    implicitHeight: segmentRow.implicitHeight + (verticalPadding * 2)

    ButtonMethodRegistry {
        id: methodRegistry
        owner: control
        defaultTrigger: "manual"
    }

    Rectangle {
        anchors.fill: parent
        color: control.backgroundColor
        radius: control.resolvedCornerRadius
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
        SpringBehavior on spacing { motionEnabled: control.motionEnabled }
    }

    onForceBorderlessToneChanged: scheduleSyncSegmentStyles()
    onHorizontalPaddingChanged: scheduleSyncSegmentStyles()
    onVerticalPaddingChanged: scheduleSyncSegmentStyles()
    onSpacingChanged: scheduleSyncSegmentStyles()

    Connections {
        target: segmentRow
        function onChildrenChanged() {
            control.scheduleSyncSegmentStyles()
        }
    }

    Component.onDestruction: {
        for (let i = 0; i < _styledSegments.length; i++) {
            const button = _styledSegments[i]
            if (button && button._segmentStyleOwner === control)
                button._segmentStyleOwner = null
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
//     methods: [function(eventData) { ... }]
// }
