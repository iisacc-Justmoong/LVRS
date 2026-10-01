import QtQuick
import LVRS 1.0

Scale {
    id: root
    required property Item target
    // Stock controls attach to their visual slots so the input target stays fixed.
    property bool autoAttach: false
    property Item _attachedTarget: null
    property bool motionEnabled: true
    property bool pressed: false
    property bool hovered: false
    property bool focused: false
    // Authored button/menu/list families rebound only on a genuine release.
    property bool releaseOnSignal: false
    property real strength: 1.0
    readonly property bool active: motionEnabled && target !== null && target.enabled && Motion.animated
    property real pressProgress: active && pressed ? 1 : 0
    property real hoverProgress: active && (hovered || focused) ? 1 : 0
    property real releaseProgress: 0
    readonly property bool releasing: releaseAnimation.running
    readonly property real deformationProgress: !active ? 0
        : releaseOnSignal && !pressed ? releaseProgress : pressProgress
    origin.x: target ? target.width / 2 : 0
    origin.y: target ? target.height / 2 : 0
    // Bound displacement for wide rows/cards and keep layout metrics unchanged.
    xScale: !active ? 1 : 1 + strength * (Math.min(0.012, 2 / Math.max(1, target ? target.width : 1)) * hoverProgress
                         - Math.min(0.045, 4 / Math.max(1, target ? target.width : 1)) * deformationProgress)
    yScale: !active ? 1 : 1 + strength * (Math.min(0.018, 2 / Math.max(1, target ? target.height : 1)) * hoverProgress
                         - Math.min(0.09, 4 / Math.max(1, target ? target.height : 1)) * deformationProgress)

    function playRelease() {
        if (!active || !releaseOnSignal)
            return
        // Guarantee feedback for a tap completed before the first press frame.
        const initialProgress = Math.max(0.35, Math.min(1, pressProgress))
        releaseAnimation.stop()
        releaseProgress = initialProgress
        releaseAnimation.restart()
    }

    function stopRelease() {
        releaseAnimation.stop()
        releaseProgress = 0
    }
    onPressedChanged: {
        if (pressed)
            stopRelease()
    }
    onActiveChanged: {
        if (!active)
            stopRelease()
    }

    readonly property NumberAnimation _releaseAnimation: NumberAnimation {
        id: releaseAnimation
        target: root
        property: "releaseProgress"
        to: 0
        duration: Motion.duration(Motion.buttonReleaseDuration)
        easing.type: Easing.OutBack
        easing.overshoot: Motion.overshoot
    }

    function syncAttachment() {
        if (_attachedTarget === (autoAttach ? target : null))
            return
        if (_attachedTarget) {
            const retained = []
            for (let i = 0; i < _attachedTarget.transform.length; i++)
                if (_attachedTarget.transform[i] !== root)
                    retained.push(_attachedTarget.transform[i])
            _attachedTarget.transform = retained
        }
        _attachedTarget = autoAttach ? target : null
        if (_attachedTarget)
            _attachedTarget.transform.push(root)
    }
    onTargetChanged: syncAttachment()
    onAutoAttachChanged: syncAttachment()
    Component.onCompleted: syncAttachment()

    SpringBehavior on pressProgress {
        motionEnabled: root.active
        duration: targetValue > 0.5 || root.releaseOnSignal ? Motion.pressDuration : Motion.releaseDuration
        easingType: targetValue > 0.5 || root.releaseOnSignal ? Easing.OutCubic : Easing.OutBack
    }
    SpringBehavior on hoverProgress {
        motionEnabled: root.active
        duration: Motion.hoverDuration
    }
}

// transform: LV.InteractionMotion { target: button; pressed: button.down; hovered: button.hovered }
