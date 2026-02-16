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
    property real menuOpacity: 0.5
    readonly property color resolvedMenuColor:
        Qt.rgba(menuColor.r, menuColor.g, menuColor.b, Math.max(0.0, Math.min(menuOpacity, 1.0)))

    signal itemTriggered(int index, var item)

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
                    enabled: control.itemEnabled(delegateRoot.entry)
                    onClicked: {
                        control.itemTriggered(delegateRoot.index, delegateRoot.entry)
                        if (control.autoCloseOnTrigger && !control.itemShowChevron(delegateRoot.entry))
                            control.close()
                    }
                }
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.ContextMenu { items: [{ icon: "iconname", label: "Open", key: "cmd+o" }, { type: "divider" }, { icon: "iconname", label: "Share", key: "cmd+s", hasSubmenu: true }] }
