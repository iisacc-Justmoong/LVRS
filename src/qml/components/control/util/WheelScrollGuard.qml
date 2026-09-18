import QtQuick
import LVRS 1.0

Item {
    id: root

    // Target must expose: contentY, contentHeight, height (Flickable-compatible API).
    property var targetFlickable: parent
    property bool consumeInside: true
    property real fallbackStep: Theme.gap20

    signal wheelRouted(var wheelEvent, real delta, real previousContentY, real nextContentY)

    anchors.fill: parent
    visible: true
    opacity: 0.0
    z: 1000

    function pointInsideTarget(x, y) {
        const target = targetFlickable
        if (!target || !target.visible || target.width <= 0 || target.height <= 0)
            return false
        const localPoint = target.mapFromItem(root, x, y)
        return localPoint.x >= 0 && localPoint.x <= target.width
            && localPoint.y >= 0 && localPoint.y <= target.height
    }

    function wheelDeltaPixels(wheelEvent) {
        if (!wheelEvent)
            return 0
        if (wheelEvent.pixelDelta && wheelEvent.pixelDelta.y !== 0)
            return wheelEvent.pixelDelta.y
        if (wheelEvent.angleDelta && wheelEvent.angleDelta.y !== 0)
            return (wheelEvent.angleDelta.y / 120) * fallbackStep
        return 0
    }

    function routeWheel(wheelEvent) {
        const target = targetFlickable
        if (!wheelEvent || !target)
            return
        if (!pointInsideTarget(wheelEvent.x, wheelEvent.y))
            return

        const maxContentY = Math.max(0, Number(target.contentHeight) - Number(target.height))
        const previousY = Number(target.contentY)
        let nextY = previousY
        const delta = wheelDeltaPixels(wheelEvent)

        if (maxContentY > 0) {
            if (Math.abs(delta) >= 0.001)
                nextY = Math.max(0, Math.min(maxContentY, previousY - delta))
        }

        if (Math.abs(nextY - previousY) > 0.001)
            target.contentY = nextY

        wheelRouted(wheelEvent, delta, previousY, nextY)

        if (consumeInside)
            wheelEvent.accepted = true
    }

    EventListener {
        anchors.fill: parent
        trigger: "wheel"
        action: function(wheelEvent) {
            root.routeWheel(wheelEvent)
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.WheelScrollGuard { targetFlickable: innerFlickable; consumeInside: true }
