import QtQuick
import LVRS 1.0

Behavior {
    id: root
    property bool motionEnabled: true
    property int duration: Motion.releaseDuration
    property int easingType: Easing.OutBack
    property real overshoot: Motion.overshoot
    enabled: motionEnabled && Motion.animated && duration > 0

    NumberAnimation {
        id: animation
        duration: root.enabled ? Motion.duration(root.duration) : 0
        easing.type: root.easingType
        easing.overshoot: root.overshoot
    }

    // Settle an interrupted animation when a live preference disables motion.
    onEnabledChanged: {
        if (!enabled && animation.running)
            animation.complete()
    }
}

// LV.SpringBehavior on rotation { duration: LV.Motion.releaseDuration }
