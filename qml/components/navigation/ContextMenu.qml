pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import LVRS 1.0

Controls.Popup {
    id: control

    property var items: []
    property int itemWidth: 161
    property int itemSpacing: 0
    property int selectedIndex: -1
    property bool autoCloseOnTrigger: true
    property bool dismissOnGlobalPress: true
    property bool dismissOnGlobalContextRequest: true
    property color menuColor: Theme.contextMenuSurface
    property color dividerColor: Theme.contextMenuDivider
    property real menuOpacity: 1.0
    property bool enableOpenBounce: true
    property int openBounceDuration: 170
    property int openSettleDuration: 110
    property real openStartScale: 0.78
    property real openOvershootScale: 1.06
    property real openAnchorX: 0
    property real openAnchorY: 0
    readonly property color resolvedMenuColor:
        Qt.rgba(menuColor.r, menuColor.g, menuColor.b, Math.max(0.0, Math.min(menuOpacity, 1.0)))

    signal itemTriggered(int index, var item)
    signal itemEventTriggered(string eventName, var payload, int index, var item)

    modal: true
    dim: false
    focus: true
    padding: Theme.gap4
    closePolicy: Controls.Popup.CloseOnEscape
        | Controls.Popup.CloseOnPressOutside
        | Controls.Popup.CloseOnPressOutsideParent
        | Controls.Popup.CloseOnReleaseOutside
        | Controls.Popup.CloseOnReleaseOutsideParent
    parent: Controls.Overlay.overlay
    Controls.Overlay.modal: Item {
        anchors.fill: parent

        EventListener {
            anchors.fill: parent
            trigger: "pressed"
            acceptedButtons: Qt.AllButtons
            action: function(mouse) {
                if (!control.opened)
                    return
                const insidePopup = mouse.x >= control.x
                    && mouse.x <= control.x + control.width
                    && mouse.y >= control.y
                    && mouse.y <= control.y + control.height
                if (!insidePopup)
                    control.close()
            }
        }
    }

    transform: Scale {
        id: openBounceTransform
        origin.x: control.openAnchorX
        origin.y: control.openAnchorY
        xScale: 1.0
        yScale: 1.0
    }

    enter: Transition {
        enabled: control.enableOpenBounce
        ParallelAnimation {
            NumberAnimation {
                target: control
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: control.openBounceDuration
                easing.type: Easing.OutCubic
            }

            SequentialAnimation {
                NumberAnimation {
                    target: openBounceTransform
                    properties: "xScale,yScale"
                    from: control.openStartScale
                    to: control.openOvershootScale
                    duration: control.openBounceDuration
                    easing.type: Easing.OutCubic
                }

                NumberAnimation {
                    target: openBounceTransform
                    properties: "xScale,yScale"
                    to: 1.0
                    duration: control.openSettleDuration
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    readonly property int entryCount: {
        if (!items)
            return 0
        if (items.length !== undefined)
            return items.length
        if (items.count !== undefined)
            return items.count
        return 0
    }

    implicitWidth: itemWidth + leftPadding + rightPadding
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding

    function pointInsidePopupAtLocal(localX, localY) {
        return localX >= control.x
            && localX <= control.x + control.width
            && localY >= control.y
            && localY <= control.y + control.height
    }

    function overlayPointFromGlobal(globalX, globalY) {
        if (control.parent && control.parent.mapFromGlobal)
            return control.parent.mapFromGlobal(globalX, globalY)
        return Qt.point(globalX, globalY)
    }

    function dismissIfOutsideGlobalEvent(eventData) {
        if (!control.opened || !eventData)
            return false
        const globalX = eventData.globalX !== undefined ? Number(eventData.globalX) : Number(eventData.x)
        const globalY = eventData.globalY !== undefined ? Number(eventData.globalY) : Number(eventData.y)
        if (isNaN(globalX) || isNaN(globalY))
            return false
        const overlayPoint = control.overlayPointFromGlobal(globalX, globalY)
        if (control.pointInsidePopupAtLocal(overlayPoint.x, overlayPoint.y))
            return false
        control.close()
        return true
    }

    function entryAt(index) {
        if (!items)
            return null
        if (items.length !== undefined)
            return items[index]
        if (items.get !== undefined)
            return items.get(index)
        return null
    }

    function isDivider(entry) {
        if (!entry)
            return false
        if (entry.type !== undefined && String(entry.type).toLowerCase() === "divider")
            return true
        return entry.divider === true
    }

    function itemLabel(entry) {
        if (typeof entry === "string")
            return entry
        if (!entry || typeof entry !== "object")
            return ""
        return entry.label || entry.text || entry.title || ""
    }

    function itemShortcut(entry) {
        if (!entry || typeof entry !== "object")
            return ""
        return entry.key || entry.shortcut || ""
    }

    function itemIconName(entry) {
        if (!entry || typeof entry !== "object")
            return ""
        return entry.iconName || entry.icon || ""
    }

    function itemIconSource(entry) {
        if (!entry || typeof entry !== "object")
            return ""
        return entry.iconSource || entry.source || ""
    }

    function itemEnabled(entry) {
        if (!entry || typeof entry !== "object")
            return true
        if (entry.enabled === undefined)
            return true
        return !!entry.enabled
    }

    function itemShowChevron(entry) {
        if (!entry || typeof entry !== "object")
            return true
        if (entry.showChevron !== undefined)
            return !!entry.showChevron
        if (entry.hasSubmenu !== undefined)
            return !!entry.hasSubmenu
        return true
    }

    function itemSelectionDirection(entry) {
        if (!entry || typeof entry !== "object")
            return "right"
        if (entry.selectionDirection !== undefined)
            return entry.selectionDirection
        if (entry.direction !== undefined)
            return entry.direction
        if (entry.chevronDirection !== undefined)
            return entry.chevronDirection
        return "right"
    }

    function itemState(entry, index, menuItem) {
        if (entry && typeof entry === "object" && entry.state !== undefined)
            return entry.state
        if (entry && typeof entry === "object" && entry.selected === true)
            return menuItem.selectedState
        if (index === selectedIndex)
            return menuItem.selectedState
        if (!itemEnabled(entry))
            return menuItem.inactiveState
        return menuItem.defaultState
    }

    function itemCallback(entry) {
        if (!entry || typeof entry !== "object")
            return null
        if (entry.onTriggered !== undefined && typeof entry.onTriggered === "function")
            return entry.onTriggered
        if (entry.onClicked !== undefined && typeof entry.onClicked === "function")
            return entry.onClicked
        if (entry.handler !== undefined && typeof entry.handler === "function")
            return entry.handler
        return null
    }

    function itemEventName(entry, index) {
        if (!entry || typeof entry !== "object")
            return ""

        const explicit = entry.eventName !== undefined
            ? entry.eventName
            : (entry.event !== undefined
                   ? entry.event
                   : (entry.action !== undefined
                          ? entry.action
                          : entry.id))
        if (explicit === undefined || explicit === null)
            return ""
        return String(explicit).trim()
    }

    function itemEventPayload(entry) {
        if (!entry || typeof entry !== "object")
            return entry
        if (entry.eventPayload !== undefined)
            return entry.eventPayload
        if (entry.payload !== undefined)
            return entry.payload
        return entry
    }

    function itemEventSpecs(entry, index) {
        const specs = []
        if (!entry || typeof entry !== "object")
            return specs

        if (Array.isArray(entry.events)) {
            for (let i = 0; i < entry.events.length; i++) {
                const rawSpec = entry.events[i]
                if (typeof rawSpec === "string") {
                    const eventName = rawSpec.trim()
                    if (eventName.length > 0)
                        specs.push({ eventName: eventName, payload: itemEventPayload(entry) })
                    continue
                }

                if (!rawSpec || typeof rawSpec !== "object")
                    continue

                const rawName = rawSpec.eventName !== undefined
                    ? rawSpec.eventName
                    : (rawSpec.event !== undefined
                           ? rawSpec.event
                           : (rawSpec.name !== undefined
                                  ? rawSpec.name
                                  : rawSpec.type))
                if (rawName === undefined || rawName === null)
                    continue

                const normalizedName = String(rawName).trim()
                if (normalizedName.length === 0)
                    continue

                specs.push({
                    eventName: normalizedName,
                    payload: rawSpec.payload !== undefined ? rawSpec.payload : itemEventPayload(entry)
                })
            }
        }

        if (specs.length === 0) {
            const fallbackName = itemEventName(entry, index)
            if (fallbackName.length > 0)
                specs.push({ eventName: fallbackName, payload: itemEventPayload(entry) })
        }
        return specs
    }

    function shouldCloseOnTrigger(entry) {
        if (!entry || typeof entry !== "object")
            return control.autoCloseOnTrigger && !itemShowChevron(entry)

        if (entry.keepOpen === true || entry.preventClose === true)
            return false
        if (entry.closeOnTrigger !== undefined)
            return !!entry.closeOnTrigger
        if (entry.autoClose !== undefined)
            return !!entry.autoClose
        return control.autoCloseOnTrigger && !itemShowChevron(entry)
    }

    function emitItemEvents(index, entry) {
        const specs = itemEventSpecs(entry, index)
        for (let i = 0; i < specs.length; i++) {
            const spec = specs[i]
            if (!spec || spec.eventName === undefined || spec.eventName === null)
                continue
            const normalizedName = String(spec.eventName).trim()
            if (normalizedName.length === 0)
                continue
            itemEventTriggered(normalizedName, spec.payload, index, entry)
        }
        return specs.length > 0 ? specs[0] : null
    }

    function invokeItemCallback(index, entry, firstEventSpec) {
        const callback = itemCallback(entry)
        if (!callback)
            return false

        try {
            callback({
                index: index,
                item: entry,
                menu: control,
                eventName: firstEventSpec && firstEventSpec.eventName !== undefined ? firstEventSpec.eventName : "",
                payload: firstEventSpec ? firstEventSpec.payload : itemEventPayload(entry),
                emit: function(eventName, payload) {
                    if (eventName === undefined || eventName === null)
                        return
                    const normalizedName = String(eventName).trim()
                    if (normalizedName.length === 0)
                        return
                    control.itemEventTriggered(normalizedName, payload, index, entry)
                },
                close: function() {
                    control.close()
                }
            })
            return true
        } catch (error) {
            console.warn("ContextMenu callback failed:", error)
            return false
        }
    }

    function triggerEntry(index) {
        const entry = entryAt(index)
        if (entry === undefined || entry === null || isDivider(entry) || !itemEnabled(entry))
            return false

        const firstEventSpec = emitItemEvents(index, entry)
        invokeItemCallback(index, entry, firstEventSpec)
        itemTriggered(index, entry)

        if (shouldCloseOnTrigger(entry))
            close()
        return true
    }

    function openAt(xPos, yPos) {
        var px = xPos
        var py = yPos
        var targetWidth = Math.max(implicitWidth, width)
        var targetHeight = Math.max(implicitHeight, height)
        if (parent) {
            px = Math.max(0, Math.min(px, parent.width - targetWidth))
            py = Math.max(0, Math.min(py, parent.height - targetHeight))
        }
        x = Math.round(px)
        y = Math.round(py)
        openAnchorX = Math.max(0, Math.min(targetWidth, xPos - x))
        openAnchorY = Math.max(0, Math.min(targetHeight, yPos - y))
        // Defer open to avoid immediate close when called from press handlers.
        Qt.callLater(function() {
            control.open()
        })
    }

    function openFor(item, xPos, yPos) {
        if (!item || !parent) {
            openAt(xPos, yPos)
            return
        }
        const mapped = item.mapToItem(parent, xPos, yPos)
        openAt(mapped.x, mapped.y)
    }

    Item {
        id: globalDismissBridge
        width: 0
        height: 0
        visible: false

        EventListener {
            trigger: "globalPressed"
            enabled: control.dismissOnGlobalPress && control.opened
            includeUiHit: false
            action: function(eventData) {
                control.dismissIfOutsideGlobalEvent(eventData)
            }
        }

        EventListener {
            trigger: "globalContextRequested"
            enabled: control.dismissOnGlobalContextRequest && control.opened
            includeUiHit: false
            action: function(eventData) {
                control.dismissIfOutsideGlobalEvent(eventData)
            }
        }
    }

    background: Rectangle {
        radius: Theme.radiusSm
        color: control.resolvedMenuColor
        antialiasing: true
    }

    contentItem: Column {
        id: menuColumn
        spacing: control.itemSpacing
        width: control.itemWidth

        Repeater {
            model: control.entryCount

            delegate: Item {
                id: delegateRoot
                required property int index
                readonly property var entry: control.entryAt(index)
                readonly property bool divider: control.isDivider(entry)

                width: control.itemWidth
                implicitHeight: divider ? dividerItem.implicitHeight : menuItem.implicitHeight

                MenuDivider {
                    id: dividerItem
                    visible: delegateRoot.divider
                    width: control.itemWidth
                    dividerColor: control.dividerColor
                }

                MenuItem {
                    id: menuItem
                    visible: !delegateRoot.divider
                    width: control.itemWidth
                    itemWidth: control.itemWidth
                    state: control.itemState(delegateRoot.entry, delegateRoot.index, menuItem)
                    label: control.itemLabel(delegateRoot.entry)
                    key: control.itemShortcut(delegateRoot.entry)
                    iconName: control.itemIconName(delegateRoot.entry)
                    iconSource: control.itemIconSource(delegateRoot.entry)
                    showChevron: control.itemShowChevron(delegateRoot.entry)
                    selectionDirection: control.itemSelectionDirection(delegateRoot.entry)
                    enabled: control.itemEnabled(delegateRoot.entry)
                    onClicked: control.triggerEntry(delegateRoot.index)
                }
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ContextMenu {
//     items: [
//         {
//             id: "openRecent",
//             label: "Open Recent",
//             eventName: "menu.openRecent",
//             eventPayload: ({ source: "context-menu" }),
//             onTriggered: function(ctx) { console.log(ctx.eventName) },
//             closeOnTrigger: true
//         }
//     ]
// }
