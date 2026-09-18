pragma Singleton
import QtQuick

QtObject {
    // Shared by every LVRS control in an engine. Applications may opt out live.
    property bool enabled: true
    property bool reducedMotion: false
    property real speed: 1.0
    readonly property bool animated: enabled && !reducedMotion
    readonly property int pressDuration: 90
    readonly property int hoverDuration: 160
    readonly property int releaseDuration: 360
    readonly property int surfaceDuration: 420
    readonly property int exitDuration: 150
    readonly property int colorDuration: 130
    readonly property real overshoot: 1.45

    function duration(milliseconds) {
        if (!animated)
            return 0
        const factor = isFinite(speed) ? Math.max(0.1, Math.min(4, speed)) : 1
        return Math.max(0, Math.round(milliseconds / factor))
    }
}

// LV.Motion.reducedMotion = true; LV.Motion.speed = 0.5
