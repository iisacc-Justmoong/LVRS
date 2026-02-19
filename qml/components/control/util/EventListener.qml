pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0

Item {
    id: root

    // Supported triggers: clicked, pressed, released, entered, exited, hoverChanged, wheel, keyPressed, keyReleased, globalPressed, globalContextRequested
    property string trigger: "clicked"
    property var action: null
    property var payload: ({})
    property bool enabled: true
    property int acceptedButtons: Qt.LeftButton
    property bool macControlClickAsRight: true
    property int globalPressDedupMs: 24
    property real globalPressDedupTolerancePx: 2.0
    property int contextDedupMs: 180
    property real contextDedupTolerancePx: 2.0
    property bool includeUiHit: false
    property bool includeInputState: false
    property bool preferBackendState: false
    property bool includeBackendSummary: false
    property double lastContextTimestamp: -1
    property real lastContextX: -1
    property real lastContextY: -1
    property double lastGlobalPressTimestamp: -1
    property real lastGlobalPressX: -1
    property real lastGlobalPressY: -1
    property int lastGlobalPressButtons: 0
    property int lastGlobalPressModifiers: 0

    anchors.fill: parent
    visible: true
    opacity: 0.0
    focus: root.enabled && root.isKeyTrigger(root.trigger)
    activeFocusOnTab: root.isKeyTrigger(root.trigger)

    function isPointerTrigger(name) {
        return name === "clicked" || name === "pressed" || name === "released"
            || name === "entered" || name === "exited" || name === "hoverChanged"
    }

    function isWheelTrigger(name) {
        return name === "wheel"
    }

    function isKeyTrigger(name) {
        return name === "keyPressed" || name === "keyReleased"
    }

    function isGlobalPointerTrigger(name) {
        return name === "globalPressed" || name === "globalContextRequested"
    }

    function isContextGesture(buttons, modifiers) {
        const isRightButton = (buttons & Qt.RightButton) === Qt.RightButton
        const isMacControlClick = macControlClickAsRight
            && (buttons & Qt.LeftButton) === Qt.LeftButton
            && (modifiers & Qt.ControlModifier) === Qt.ControlModifier
        return isRightButton || isMacControlClick
    }

    function mapHasEntries(mapValue) {
        if (!mapValue)
            return false
        for (const key in mapValue)
            return true
        return false
    }

    function ensureBackendHooked() {
        if (!root.preferBackendState || !Backend || !Backend.hookUserEvents)
            return false
        if (Backend.userEventHooked === undefined)
            return true
        if (!Backend.userEventHooked)
            Backend.hookUserEvents()
        return !!Backend.userEventHooked
    }

    function resolveBackendSummary() {
        if (!root.includeBackendSummary || !root.preferBackendState || !Backend || !Backend.hookedUserEventSummary)
            return ({})
        root.ensureBackendHooked()
        return Backend.hookedUserEventSummary()
    }

    function globalMousePayload(x, y, buttons, modifiers) {
        const data = {
            x: x,
            y: y,
            globalX: x,
            globalY: y,
            buttons: buttons,
            modifiers: modifiers,
            isGlobal: true
        }
        let inputState = undefined
        if (root.includeInputState)
            inputState = root.resolveInputState()
        if (root.includeUiHit)
            data.ui = root.resolveUiAt(x, y, inputState)
        if (root.includeInputState)
            data.input = inputState
        if (root.includeBackendSummary)
            data.backend = root.resolveBackendSummary()
        return data
    }

    function resolveUiAt(globalX, globalY, inputState) {
        if (!root.includeUiHit)
            return ({})
        if (RuntimeEvents && RuntimeEvents.hitTestUiAt) {
            const resolved = RuntimeEvents.hitTestUiAt(globalX, globalY)
            if (root.mapHasEntries(resolved))
                return resolved
        }
        if (!root.includeInputState)
            return ({})
        const resolvedInputState = inputState !== undefined ? inputState : root.resolveInputState()
        const pointerUi = resolvedInputState && resolvedInputState.pointerUi ? resolvedInputState.pointerUi : ({})
        return root.mapHasEntries(pointerUi) ? pointerUi : ({})
    }

    function resolveInputState() {
        if (root.preferBackendState && Backend && Backend.currentUserInputState) {
            root.ensureBackendHooked()
            const backendState = Backend.currentUserInputState()
            if (root.mapHasEntries(backendState))
                return backendState
        }
        if (RuntimeEvents && RuntimeEvents.inputState) {
            const runtimeState = RuntimeEvents.inputState()
            if (root.mapHasEntries(runtimeState))
                return runtimeState
        }
        return ({})
    }

    function pointerGlobalPosition(mouse) {
        if (mouse && mouse.globalX !== undefined && mouse.globalY !== undefined)
            return Qt.point(mouse.globalX, mouse.globalY)
        const localX = mouse && mouse.x !== undefined ? mouse.x : 0
        const localY = mouse && mouse.y !== undefined ? mouse.y : 0
        if (root.mapToGlobal)
            return root.mapToGlobal(Qt.point(localX, localY))
        return Qt.point(localX, localY)
    }

    function pointerPayload(mouse) {
        const globalPoint = pointerGlobalPosition(mouse)
        const data = {
            x: mouse && mouse.x !== undefined ? mouse.x : globalPoint.x,
            y: mouse && mouse.y !== undefined ? mouse.y : globalPoint.y,
            globalX: globalPoint.x,
            globalY: globalPoint.y,
            button: mouse && mouse.button !== undefined ? mouse.button : Qt.NoButton,
            buttons: mouse && mouse.buttons !== undefined ? mouse.buttons : Qt.NoButton,
            modifiers: mouse && mouse.modifiers !== undefined ? mouse.modifiers : Qt.NoModifier,
            isGlobal: false
        }
        let inputState = undefined
        if (root.includeInputState)
            inputState = root.resolveInputState()
        if (root.includeUiHit)
            data.ui = root.resolveUiAt(globalPoint.x, globalPoint.y, inputState)
        if (root.includeInputState)
            data.input = inputState
        if (root.includeBackendSummary)
            data.backend = root.resolveBackendSummary()
        return data
    }

    function isDuplicateContextEvent(x, y, nowMs) {
        if (lastContextTimestamp < 0)
            return false
        if ((nowMs - lastContextTimestamp) > contextDedupMs)
            return false
        return Math.abs(lastContextX - x) <= contextDedupTolerancePx
            && Math.abs(lastContextY - y) <= contextDedupTolerancePx
    }

    function isDuplicateGlobalPress(x, y, buttons, modifiers, nowMs) {
        if (lastGlobalPressTimestamp < 0)
            return false
        if ((nowMs - lastGlobalPressTimestamp) > globalPressDedupMs)
            return false
        if (Math.abs(lastGlobalPressX - x) > globalPressDedupTolerancePx)
            return false
        if (Math.abs(lastGlobalPressY - y) > globalPressDedupTolerancePx)
            return false
        return lastGlobalPressButtons === buttons && lastGlobalPressModifiers === modifiers
    }

    function rememberGlobalPress(x, y, buttons, modifiers, nowMs) {
        lastGlobalPressTimestamp = nowMs
        lastGlobalPressX = x
        lastGlobalPressY = y
        lastGlobalPressButtons = buttons
        lastGlobalPressModifiers = modifiers
    }

    function fireGlobalContext(x, y, buttons, modifiers, reason, source) {
        const nowMs = Date.now()
        if (root.isDuplicateContextEvent(x, y, nowMs))
            return
        root.lastContextTimestamp = nowMs
        root.lastContextX = x
        root.lastContextY = y
        const data = root.globalMousePayload(x, y, buttons, modifiers)
        data.reason = reason
        data.source = source
        root.fire(data)
    }

    function ensureKeyFocus() {
        if (root.enabled && root.isKeyTrigger(root.trigger))
            root.forceActiveFocus()
    }

    function fire(eventData) {
        if (!root.enabled || !root.action)
            return
        const callback = root.action
        if (typeof callback !== "function")
            return
        callback(eventData !== undefined ? eventData : root.payload)
    }

    onTriggerChanged: root.ensureKeyFocus()
    onEnabledChanged: root.ensureKeyFocus()
    Component.onCompleted: {
        root.ensureKeyFocus()
    }

    Connections {
        target: RuntimeEvents
        enabled: root.enabled && root.isGlobalPointerTrigger(root.trigger)
        function onMousePressed(x, y, buttons, modifiers) {
            if (root.trigger === "globalPressed") {
                const nowMs = Date.now()
                if (root.isDuplicateGlobalPress(x, y, buttons, modifiers, nowMs))
                    return
                root.rememberGlobalPress(x, y, buttons, modifiers, nowMs)
                root.fire(root.globalMousePayload(x, y, buttons, modifiers))
                return
            }
            if (root.trigger !== "globalContextRequested")
                return
            if (!root.isContextGesture(buttons, modifiers))
                return
            root.fireGlobalContext(x, y, buttons, modifiers, -1, "mouse")
        }

        function onContextRequested(x, y, modifiers, reason) {
            if (root.trigger !== "globalContextRequested")
                return
            const inferredButtons = reason === 0 ? Qt.RightButton : Qt.NoButton
            root.fireGlobalContext(x, y, inferredButtons, modifiers, reason, "context")
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.enabled && root.isPointerTrigger(root.trigger)
        hoverEnabled: root.trigger === "entered" || root.trigger === "exited" || root.trigger === "hoverChanged"
        acceptedButtons: root.acceptedButtons
        propagateComposedEvents: true

        onClicked: function(mouse) {
            if (root.trigger === "clicked")
                root.fire(root.pointerPayload(mouse))
        }
        onPressed: function(mouse) {
            if (root.trigger === "pressed")
                root.fire(root.pointerPayload(mouse))
        }
        onReleased: function(mouse) {
            if (root.trigger === "released")
                root.fire(root.pointerPayload(mouse))
        }
        onEntered: {
            if (root.trigger === "entered")
                root.fire()
        }
        onExited: {
            if (root.trigger === "exited")
                root.fire()
        }
        onContainsMouseChanged: {
            if (root.trigger === "hoverChanged")
                root.fire({ containsMouse: containsMouse })
        }
    }

    WheelHandler {
        enabled: root.enabled && root.isWheelTrigger(root.trigger)
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: function(event) {
            root.fire(event)
        }
    }

    Keys.onPressed: function(event) {
        if (root.enabled && root.trigger === "keyPressed")
            root.fire(event)
    }
    Keys.onReleased: function(event) {
        if (root.enabled && root.trigger === "keyReleased")
            root.fire(event)
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Label { LV.EventListener { trigger: "clicked"; action: () => console.log("click") } }
