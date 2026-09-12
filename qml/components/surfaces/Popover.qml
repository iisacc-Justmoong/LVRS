import QtQuick
import QtQuick.Controls as Controls
import LVRS 1.0

Controls.Popup {
    id: root
    property bool motionEnabled: true

    property color primaryColor: Theme.primary
    property Item backdropSource: Controls.ApplicationWindow.contentItem
    property Item backdropBackground: Controls.ApplicationWindow.window
        ? Controls.ApplicationWindow.window.background : null
    property real cornerRadius: Theme.materialPanelRadius
    property color surfaceColor: Theme.materialTint
    property real surfaceOpacity: Theme.materialGlassOpacity
    readonly property alias material: surface

    parent: Controls.Overlay.overlay
    popupType: Controls.Popup.Item
    padding: Theme.gap12
    margins: Theme.gap8
    modal: false
    dim: false
    focus: true
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside
    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: root.motionEnabled ? Motion.duration(Motion.hoverDuration) : 0 }
            NumberAnimation {
                property: "scale"; from: Motion.animated && root.motionEnabled ? 0.92 : 1; to: 1
                duration: root.motionEnabled ? Motion.duration(Motion.surfaceDuration) : 0
                easing.type: Easing.OutBack; easing.overshoot: Motion.overshoot
            }
        }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: root.motionEnabled ? Motion.duration(Motion.exitDuration) : 0 }
    }
    background: PanelMaterial {
        id: surface
        objectName: "popoverMaterial"
        primaryColor: root.primaryColor
        color: root.surfaceColor
        tintOpacity: root.surfaceOpacity
        radius: root.cornerRadius
        backdropSource: root.backdropSource
        backdropBackground: root.backdropBackground
    }
}

// LV.Popover { width: 320; height: 180; contentItem: SettingsView {} }
