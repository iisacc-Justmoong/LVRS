pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import LVRS 1.0

Controls.Popup {
    id: control

    property var items: []
    property int itemWidth: Theme.scaleMetric(161)
    property int itemSpacing: 0
    property int selectedIndex: -1
    property bool autoCloseOnTrigger: true
    property bool dismissOnGlobalPress: true
    property bool dismissOnGlobalContextRequest: true
    property color menuColor: Theme.contextMenuSurface
    property color dividerColor: Theme.contextMenuDivider
    property real menuOpacity: 1.0
    property int edgeMargin: Theme.gap4
    property bool enableOpenBounce: true
    property bool autoTuneByBackend: true
    property int openBounceDuration: 170
    property int openSettleDuration: 110
    property real openStartScale: 0.86
    property real openOvershootScale: 1.04
    property real openAnchorX: 0
    property real openAnchorY: 0
    property real openStartX: 0
    property real openStartY: 0
    property real openTargetX: 0
    property real openTargetY: 0
    readonly property int directionRight: 0
    readonly property int directionLeft: 1
    readonly property int directionDown: 2
    readonly property int directionUp: 3
    property int openHorizontalDirection: directionRight
    property int openVerticalDirection: directionDown
    readonly property var backendRuntimeProfile: Platform.runtimeProfile(Platform.canonicalOs)
    readonly property bool backendTransitionReady: backendRuntimeProfile.backendFeatureReady !== false
    readonly property real backendTransitionSpeedFactor: {
        if (!autoTuneByBackend)
            return 1.0
        const backendName = backendRuntimeProfile.backend !== undefined && backendRuntimeProfile.backend !== null
            ? String(backendRuntimeProfile.backend).toLowerCase()
            : "default"
        const mobile = backendRuntimeProfile.mobile === true
        if (mobile && backendName === "vulkan")
            return 0.82
        if (mobile)
            return 0.90
        if (backendName === "default")
            return 0.88
        return 1.0
    }
    readonly property bool resolvedOpenBounceEnabled: enableOpenBounce
        && (!autoTuneByBackend || backendTransitionReady)
    readonly property int resolvedOpenBounceDuration: Math.max(0, Math.round(openBounceDuration * backendTransitionSpeedFactor))
    readonly property int resolvedOpenSettleDuration: Math.max(0, Math.round(openSettleDuration * backendTransitionSpeedFactor))
    readonly property int resolvedOpenReboundDuration: resolvedOpenBounceDuration + resolvedOpenSettleDuration
    readonly property real resolvedOpenStartScale: Math.max(0.01, autoTuneByBackend && backendRuntimeProfile.mobile === true
        ? (openStartScale + 0.04)
        : openStartScale)
    readonly property real resolvedOpenOvershootScale: Math.max(1.0, autoTuneByBackend && backendRuntimeProfile.mobile === true
        ? (openOvershootScale - 0.02)
        : openOvershootScale)
    readonly property real resolvedOpenBackOvershoot: Math.max(0.15, Math.min(0.95, (resolvedOpenOvershootScale - 1.0) * 8.0))
    readonly property color resolvedMenuColor:
        Qt.rgba(menuColor.r, menuColor.g, menuColor.b, Math.max(0.0, Math.min(menuOpacity, 1.0)))
    property real requestedPopupWidth: 0
    property bool synchronizingResolvedWidth: false
    readonly property int minimumItemWidth: Math.max(0, Math.round(Number(itemWidth) || 0))
    readonly property int implicitItemContentWidth: Math.max(minimumItemWidth, Math.round(Number(contentWidthProbe.implicitWidth) || 0))
    readonly property int requestedPopupWidthValue: {
        const popupWidth = Number(requestedPopupWidth)
        if (!Number.isFinite(popupWidth) || popupWidth <= 0)
            return 0
        return Math.max(0, Math.round(popupWidth))
    }
    readonly property int resolvedPopupWidth: Math.max(implicitWidth, requestedPopupWidthValue)
    readonly property int explicitContentWidth: {
        const popupWidth = Number(control.resolvedPopupWidth)
        if (!Number.isFinite(popupWidth) || popupWidth <= 0)
            return 0
        return Math.max(0, Math.round(popupWidth - control.leftPadding - control.rightPadding))
    }
    readonly property int resolvedItemWidth: Math.max(implicitItemContentWidth, explicitContentWidth)

    signal itemTriggered(int index, var item)
    signal itemEventTriggered(string eventName, var payload, int index, var item)

    modal: true
    dim: false
    focus: true
    transformOrigin: Item.TopLeft
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

    enter: Transition {
        ParallelAnimation {
            NumberAnimation {
                target: control
                property: "opacity"
                from: 0.0
                to: 1.0
                duration: control.resolvedOpenBounceEnabled ? control.resolvedOpenReboundDuration : 0
                easing.type: Easing.OutCubic
            }

            NumberAnimation {
                target: control
                property: "scale"
                from: control.resolvedOpenBounceEnabled ? control.resolvedOpenStartScale : 1.0
                to: 1.0
                duration: control.resolvedOpenBounceEnabled ? control.resolvedOpenReboundDuration : 0
                easing.type: Easing.OutBack
                easing.overshoot: control.resolvedOpenBackOvershoot
            }

            NumberAnimation {
                target: control
                property: "x"
                from: control.openStartX
                to: control.openTargetX
                duration: control.resolvedOpenBounceEnabled ? control.resolvedOpenReboundDuration : 0
                easing.type: Easing.OutQuart
            }

            NumberAnimation {
                target: control
                property: "y"
                from: control.openStartY
                to: control.openTargetY
                duration: control.resolvedOpenBounceEnabled ? control.resolvedOpenReboundDuration : 0
                easing.type: Easing.OutQuart
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

    implicitWidth: implicitItemContentWidth + leftPadding + rightPadding
    implicitHeight: contentItem.implicitHeight + topPadding + bottomPadding

    function syncResolvedWidth() {
        if (synchronizingResolvedWidth)
            return

        const targetWidth = Number(resolvedPopupWidth)
        if (!Number.isFinite(targetWidth) || targetWidth <= 0)
            return
        if (Math.abs(Number(width) - targetWidth) < 0.01)
            return

        synchronizingResolvedWidth = true
        width = targetWidth
        synchronizingResolvedWidth = false
    }

    onWidthChanged: {
        if (synchronizingResolvedWidth)
            return
        requestedPopupWidth = width
        Qt.callLater(syncResolvedWidth)
    }

    onImplicitWidthChanged: Qt.callLater(syncResolvedWidth)

    Component.onCompleted: {
        requestedPopupWidth = width
        Qt.callLater(syncResolvedWidth)
    }

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
        const rawShortcut = entry.key !== undefined
            ? entry.key
            : (entry.shortcut !== undefined
                   ? entry.shortcut
                   : entry.keyText)
        if (rawShortcut === undefined || rawShortcut === null)
            return ""
        return String(rawShortcut)
    }

    function itemKeyVisible(entry) {
        if (!entry || typeof entry !== "object")
            return false
        if (entry.keyVisible !== undefined)
            return !!entry.keyVisible
        if (entry.shortcutVisible !== undefined)
            return !!entry.shortcutVisible
        if (entry.showShortcut !== undefined)
            return !!entry.showShortcut
        return itemShortcut(entry).trim().length > 0
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
            return false
        if (entry.showChevron !== undefined)
            return !!entry.showChevron
        return true
    }

    function itemHasChildItems(entry) {
        if (!entry || typeof entry !== "object")
            return false
        if (entry.hasChildItems !== undefined)
            return !!entry.hasChildItems
        if (entry.hasSubmenu !== undefined)
            return !!entry.hasSubmenu

        const children = entry.children !== undefined ? entry.children : entry.submenu
        if (children === undefined || children === null)
            return false
        if (Array.isArray(children))
            return children.length > 0
        if (children.length !== undefined)
            return Number(children.length) > 0
        if (children.count !== undefined)
            return Number(children.count) > 0
        return false
    }

    function itemEffectiveChevron(entry) {
        return itemShowChevron(entry) && itemHasChildItems(entry)
    }

    function itemExpanded(entry) {
        if (!entry || typeof entry !== "object")
            return false
        if (entry.expanded !== undefined)
            return !!entry.expanded
        if (entry.opened !== undefined)
            return !!entry.opened
        return false
    }

    function itemSelectionDirection(entry) {
        if (!entry || typeof entry !== "object")
            return "auto"
        if (entry.selectionDirection !== undefined)
            return entry.selectionDirection
        if (entry.direction !== undefined)
            return entry.direction
        if (entry.chevronDirection !== undefined)
            return entry.chevronDirection
        return "auto"
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
            return control.autoCloseOnTrigger

        if (entry.keepOpen === true || entry.preventClose === true)
            return false
        if (entry.closeOnTrigger !== undefined)
            return !!entry.closeOnTrigger
        if (entry.autoClose !== undefined)
            return !!entry.autoClose
        return control.autoCloseOnTrigger && !itemEffectiveChevron(entry)
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
        var targetWidth = Math.max(implicitWidth, width)
        var targetHeight = Math.max(implicitHeight, height)
        if (!parent) {
            openHorizontalDirection = directionRight
            openVerticalDirection = directionDown
            openTargetX = Math.round(xPos)
            openTargetY = Math.round(yPos)
            x = openTargetX
            y = openTargetY
            openAnchorX = Math.max(0, Math.min(targetWidth, xPos - openTargetX))
            openAnchorY = Math.max(0, Math.min(targetHeight, yPos - openTargetY))
        } else {
            const placement = resolveOpenPlacement(xPos,
                                                   yPos,
                                                   targetWidth,
                                                   targetHeight,
                                                   parent.width,
                                                   parent.height)
            openHorizontalDirection = placement.horizontalDirection
            openVerticalDirection = placement.verticalDirection
            openTargetX = placement.x
            openTargetY = placement.y
            x = openTargetX
            y = openTargetY
            openAnchorX = placement.anchorX
            openAnchorY = placement.anchorY
        }
        const startScale = resolvedOpenBounceEnabled ? resolvedOpenStartScale : 1.0
        openStartX = openTargetX + openAnchorX * (1.0 - startScale)
        openStartY = openTargetY + openAnchorY * (1.0 - startScale)
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

    function clampValue(value, minimum, maximum) {
        return Math.max(minimum, Math.min(value, maximum))
    }

    function resolvedEdgeMargin() {
        const numericMargin = Number(edgeMargin)
        if (!Number.isFinite(numericMargin))
            return 0
        return Math.max(0, Math.round(numericMargin))
    }

    function resolveOpenPlacement(anchorX, anchorY, targetWidth, targetHeight, viewportWidth, viewportHeight) {
        const menuWidth = Math.max(0, Number(targetWidth) || 0)
        const menuHeight = Math.max(0, Number(targetHeight) || 0)
        const viewWidth = Math.max(0, Number(viewportWidth) || 0)
        const viewHeight = Math.max(0, Number(viewportHeight) || 0)
        const margin = resolvedEdgeMargin()

        const spaceRight = viewWidth - margin - anchorX
        const spaceLeft = anchorX - margin
        const spaceDown = viewHeight - margin - anchorY
        const spaceUp = anchorY - margin

        var horizontalDirection = directionRight
        if (spaceRight >= menuWidth) {
            horizontalDirection = directionRight
        } else if (spaceLeft >= menuWidth) {
            horizontalDirection = directionLeft
        } else {
            horizontalDirection = spaceLeft > spaceRight ? directionLeft : directionRight
        }

        var verticalDirection = directionDown
        if (spaceDown >= menuHeight) {
            verticalDirection = directionDown
        } else if (spaceUp >= menuHeight) {
            verticalDirection = directionUp
        } else {
            verticalDirection = spaceUp > spaceDown ? directionUp : directionDown
        }

        var rawX = horizontalDirection === directionLeft ? anchorX - menuWidth : anchorX
        var rawY = verticalDirection === directionUp ? anchorY - menuHeight : anchorY

        const minX = margin
        const maxX = viewWidth - margin - menuWidth
        const minY = margin
        const maxY = viewHeight - margin - menuHeight

        if (maxX >= minX) {
            rawX = clampValue(rawX, minX, maxX)
        } else {
            rawX = Math.max(0, Math.min(rawX, viewWidth - menuWidth))
        }

        if (maxY >= minY) {
            rawY = clampValue(rawY, minY, maxY)
        } else {
            rawY = Math.max(0, Math.min(rawY, viewHeight - menuHeight))
        }

        const resolvedX = Math.round(rawX)
        const resolvedY = Math.round(rawY)

        return {
            x: resolvedX,
            y: resolvedY,
            horizontalDirection: horizontalDirection,
            verticalDirection: verticalDirection,
            anchorX: Math.max(0, Math.min(menuWidth, anchorX - resolvedX)),
            anchorY: Math.max(0, Math.min(menuHeight, anchorY - resolvedY))
        }
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

    Item {
        id: widthProbeHost
        visible: false
        width: 0
        height: 0

        Column {
            id: contentWidthProbe
            visible: false
            spacing: control.itemSpacing

            Repeater {
                model: control.entryCount

                delegate: Item {
                    id: probeDelegate
                    required property int index
                    readonly property var entry: control.entryAt(index)
                    readonly property bool divider: control.isDivider(entry)

                    width: implicitWidth
                    height: implicitHeight
                    implicitWidth: divider ? control.minimumItemWidth : probeMenuItem.implicitWidth
                    implicitHeight: divider ? 0 : probeMenuItem.implicitHeight

                    MenuItem {
                        id: probeMenuItem
                        visible: !probeDelegate.divider
                        itemWidth: control.minimumItemWidth
                        state: control.itemState(probeDelegate.entry, probeDelegate.index, probeMenuItem)
                        label: control.itemLabel(probeDelegate.entry)
                        key: control.itemShortcut(probeDelegate.entry)
                        keyVisible: control.itemKeyVisible(probeDelegate.entry)
                        iconName: control.itemIconName(probeDelegate.entry)
                        iconSource: control.itemIconSource(probeDelegate.entry)
                        showChevron: control.itemShowChevron(probeDelegate.entry)
                        hasChildItems: control.itemHasChildItems(probeDelegate.entry)
                        expanded: control.itemExpanded(probeDelegate.entry)
                        selectionDirection: control.itemSelectionDirection(probeDelegate.entry)
                        enabled: control.itemEnabled(probeDelegate.entry)
                    }
                }
            }
        }
    }

    contentItem: Column {
        id: menuColumn
        spacing: control.itemSpacing
        width: control.resolvedItemWidth

        Repeater {
            model: control.entryCount

            delegate: Item {
                id: delegateRoot
                required property int index
                readonly property var entry: control.entryAt(index)
                readonly property bool divider: control.isDivider(entry)

                width: control.resolvedItemWidth
                implicitWidth: divider ? control.minimumItemWidth : menuItem.implicitWidth
                implicitHeight: divider ? dividerItem.implicitHeight : menuItem.implicitHeight

                MenuDivider {
                    id: dividerItem
                    visible: delegateRoot.divider
                    width: control.resolvedItemWidth
                    dividerColor: control.dividerColor
                }

                MenuItem {
                    id: menuItem
                    visible: !delegateRoot.divider
                    width: control.resolvedItemWidth
                    itemWidth: control.minimumItemWidth
                    state: control.itemState(delegateRoot.entry, delegateRoot.index, menuItem)
                    label: control.itemLabel(delegateRoot.entry)
                    key: control.itemShortcut(delegateRoot.entry)
                    keyVisible: control.itemKeyVisible(delegateRoot.entry)
                    iconName: control.itemIconName(delegateRoot.entry)
                    iconSource: control.itemIconSource(delegateRoot.entry)
                    showChevron: control.itemShowChevron(delegateRoot.entry)
                    hasChildItems: control.itemHasChildItems(delegateRoot.entry)
                    expanded: control.itemExpanded(delegateRoot.entry)
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
