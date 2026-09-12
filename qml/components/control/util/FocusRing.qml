import QtQuick
import LVRS 1.0

Rectangle {
    id: root
    property bool active: false
    property bool motionEnabled: true
    color: "transparent"
    border.width: Theme.scaleRealMetric(1.5)
    border.color: Theme.primary
    radius: Theme.radiusSm
    opacity: active ? 1 : 0
    scale: active ? 1 : 0.96
    z: 100
    SpringBehavior on opacity {
        motionEnabled: root.motionEnabled
        duration: Motion.hoverDuration
        easingType: Easing.OutCubic
    }
    SpringBehavior on scale { motionEnabled: root.motionEnabled }
}

// LV.FocusRing { anchors.fill: parent; active: field.activeFocus }
