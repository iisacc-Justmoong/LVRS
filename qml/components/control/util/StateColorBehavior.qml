import QtQuick
import LVRS 1.0

Behavior {
    id: root
    property bool motionEnabled: true
    enabled: motionEnabled && Motion.animated
    ColorAnimation {
        id: animation
        duration: root.enabled ? Motion.duration(Motion.colorDuration) : 0
        easing.type: Easing.OutCubic
    }
    onEnabledChanged: {
        if (!enabled && animation.running)
            animation.complete()
    }
}

// LV.StateColorBehavior on color {}
