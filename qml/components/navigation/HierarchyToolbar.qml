pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: control

    property int horizontalPadding: Theme.gap8
    property int verticalPadding: Theme.gap2
    property int spacing: Theme.gapNone
    property color backgroundColor: Theme.subSurface
    property real backgroundOpacity: 0.88

    // Array-first API:
    // [
    //   {
    //     id: "structure",
    //     iconName: "projectStructure",
    //     iconSource: "qrc:/...",
    //     enabled: true,
    //     visible: true,
    //     selected: false,
    //     eventName: "hierarchy.structure",
    //     eventPayload: ({ source: "toolbar" }),
    //     events: ["hierarchy.structure", { name: "analytics.click", payload: ({ slot: 0 }) }],
    //     onClicked: function(ctx) {}
    //   }
    // ]
    property var buttonItems: []
    readonly property int itemCount: {
        if (!buttonItems)
            return 0
        if (buttonItems.length !== undefined)
            return buttonItems.length
        if (buttonItems.count !== undefined)
            return buttonItems.count
        return 0
    }
    readonly property bool usingItemModel: itemCount > 0
    readonly property int buttonCount: collectButtons().length

    property var activeButton: null
    property var activeButtonId: -1
    property int activeIndex: -1

    signal activeChanged(var button, var buttonId, int index)
    signal buttonTriggered(var button, var buttonId, int index, var item)
    signal buttonEventTriggered(string eventName, var payload, int index, var item, var buttonId)

    default property alias buttons: manualButtons.data

    function itemAt(index) {
        if (!buttonItems)
            return null
        if (buttonItems.length !== undefined)
            return buttonItems[index]
        if (buttonItems.get !== undefined)
            return buttonItems.get(index)
        return null
    }

    function itemButtonId(entry, index) {
        if (!entry || typeof entry !== "object")
            return index
        if (entry.buttonId !== undefined)
            return entry.buttonId
        if (entry.id !== undefined)
            return entry.id
        if (entry.key !== undefined)
            return entry.key
        return index
    }

    function itemIconName(entry) {
        if (typeof entry === "string")
            return entry
        if (!entry || typeof entry !== "object")
            return ""
        if (entry.iconName !== undefined)
            return entry.iconName
        if (entry.icon !== undefined)
            return entry.icon
        return ""
    }

    function itemIconSource(entry) {
        if (!entry || typeof entry !== "object")
            return ""
        if (entry.iconSource !== undefined)
            return entry.iconSource
        if (entry.source !== undefined)
            return entry.source
        if (entry.url !== undefined)
            return entry.url
        return ""
    }

    function itemEnabled(entry) {
        if (!entry || typeof entry !== "object")
            return true
        if (entry.enabled === undefined)
            return true
        return !!entry.enabled
    }

    function itemVisible(entry) {
        if (!entry || typeof entry !== "object")
            return true
        if (entry.visible === undefined)
            return true
        return !!entry.visible
    }

    function itemSelected(entry) {
        if (!entry || typeof entry !== "object")
            return false
        if (entry.selected !== undefined)
            return !!entry.selected
        if (entry.active !== undefined)
            return !!entry.active
        return false
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

    function itemEventName(entry) {
        if (!entry || typeof entry !== "object")
            return ""
        const explicit = entry.eventName !== undefined
            ? entry.eventName
            : (entry.event !== undefined
                   ? entry.event
                   : entry.action)
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

    function itemEventSpecs(entry) {
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
            const fallbackName = itemEventName(entry)
            if (fallbackName.length > 0)
                specs.push({ eventName: fallbackName, payload: itemEventPayload(entry) })
        }
        return specs
    }

    function collectButtons() {
        const sourceRow = usingItemModel ? modelButtons : manualButtons
        const result = []
        for (let i = 0; i < sourceRow.children.length; i++) {
            const child = sourceRow.children[i]
            if (child && child.__isToolbarButton === true)
                result.push(child)
        }
        return result
    }

    function buttonAt(index) {
        const buttons = collectButtons()
        if (index < 0 || index >= buttons.length)
            return null
        return buttons[index]
    }

    function indexOfButton(button) {
        const buttons = collectButtons()
        for (let i = 0; i < buttons.length; i++) {
            if (buttons[i] === button)
                return i
        }
        return -1
    }

    function resolveById(buttonId) {
        const buttons = collectButtons()
        for (let i = 0; i < buttons.length; i++) {
            const button = buttons[i]
            if (button && button.buttonId === buttonId)
                return button
        }
        return null
    }

    function firstSelectedButton() {
        if (!usingItemModel)
            return null
        const buttons = collectButtons()
        for (let i = 0; i < buttons.length; i++) {
            const button = buttons[i]
            const entry = itemAt(i)
            if (button && button.enabled && itemSelected(entry))
                return button
        }
        return null
    }

    function firstEnabledButton() {
        const buttons = collectButtons()
        for (let i = 0; i < buttons.length; i++) {
            const button = buttons[i]
            if (button && button.enabled)
                return button
        }
        return buttons.length > 0 ? buttons[0] : null
    }

    function registerButton(button) {
        if (!button)
            return
        if (button.toolbar !== control)
            button.toolbar = control
        normalizeActiveButton()
    }

    function emitEntryEvents(index, entry, buttonId) {
        const specs = itemEventSpecs(entry)
        for (let i = 0; i < specs.length; i++) {
            const spec = specs[i]
            if (!spec || spec.eventName === undefined || spec.eventName === null)
                continue
            const normalizedName = String(spec.eventName).trim()
            if (normalizedName.length === 0)
                continue
            buttonEventTriggered(normalizedName, spec.payload, index, entry, buttonId)
        }
        return specs.length > 0 ? specs[0] : null
    }

    function invokeEntryCallback(index, entry, button, buttonId, firstEventSpec) {
        const callback = itemCallback(entry)
        if (!callback)
            return false

        try {
            callback({
                index: index,
                item: entry,
                button: button,
                buttonId: buttonId,
                toolbar: control,
                eventName: firstEventSpec && firstEventSpec.eventName !== undefined ? firstEventSpec.eventName : "",
                payload: firstEventSpec ? firstEventSpec.payload : itemEventPayload(entry),
                emit: function(eventName, payload) {
                    if (eventName === undefined || eventName === null)
                        return
                    const normalizedName = String(eventName).trim()
                    if (normalizedName.length === 0)
                        return
                    control.buttonEventTriggered(normalizedName, payload, index, entry, buttonId)
                },
                activate: function(nextIndex) {
                    control.triggerIndex(nextIndex)
                }
            })
            return true
        } catch (error) {
            console.warn("HierarchyToolbar callback failed:", error)
            return false
        }
    }

    function dispatchEntryTrigger(button, index) {
        const buttonId = button ? button.buttonId : index
        const entry = usingItemModel ? itemAt(index) : null
        if (usingItemModel && (!entry || !itemEnabled(entry)))
            return false

        buttonTriggered(button, buttonId, index, entry)
        const firstSpec = emitEntryEvents(index, entry, buttonId)
        invokeEntryCallback(index, entry, button, buttonId, firstSpec)
        return true
    }

    function requestActivate(button) {
        if (!button || !button.enabled)
            return
        if (button.toolbar !== control)
            button.toolbar = control

        const index = indexOfButton(button)
        if (index < 0)
            return

        const changed = activeButton !== button
        activeButton = button
        activeButtonId = button.buttonId
        activeIndex = index
        if (changed)
            activeChanged(button, activeButtonId, index)

        dispatchEntryTrigger(button, index)
    }

    function normalizeActiveButton() {
        const buttons = collectButtons()
        if (buttons.length === 0) {
            activeButton = null
            activeButtonId = -1
            activeIndex = -1
            return
        }

        for (let i = 0; i < buttons.length; i++) {
            const button = buttons[i]
            if (button && button.toolbar !== control)
                button.toolbar = control
        }

        let targetButton = activeButton
        if (activeButtonId !== undefined && activeButtonId !== null) {
            const byId = resolveById(activeButtonId)
            if (byId)
                targetButton = byId
        }
        if (!targetButton || buttons.indexOf(targetButton) === -1 || !targetButton.enabled)
            targetButton = firstSelectedButton()
        if (!targetButton || buttons.indexOf(targetButton) === -1 || !targetButton.enabled)
            targetButton = firstEnabledButton()

        activeButton = targetButton
        activeButtonId = targetButton ? targetButton.buttonId : -1
        activeIndex = targetButton ? indexOfButton(targetButton) : -1
    }

    function triggerIndex(index) {
        const button = buttonAt(index)
        if (!button || !button.enabled)
            return false
        requestActivate(button)
        return true
    }

    onActiveButtonIdChanged: Qt.callLater(normalizeActiveButton)
    onButtonItemsChanged: Qt.callLater(normalizeActiveButton)

    implicitWidth: rowContainer.implicitWidth + (horizontalPadding * 2)
    implicitHeight: rowContainer.implicitHeight + (verticalPadding * 2)

    Rectangle {
        anchors.fill: parent
        color: control.backgroundColor
        opacity: control.backgroundOpacity
    }

    RowLayout {
        id: rowContainer
        anchors.fill: parent
        anchors.leftMargin: control.horizontalPadding
        anchors.rightMargin: control.horizontalPadding
        anchors.topMargin: control.verticalPadding
        anchors.bottomMargin: control.verticalPadding
        spacing: control.spacing

        Row {
            id: modelButtons
            visible: control.usingItemModel
            spacing: control.spacing

            Repeater {
                model: control.itemCount

                delegate: ToolbarButton {
                    required property int index
                    readonly property var toolbarEntry: control.itemAt(index)

                    buttonId: control.itemButtonId(toolbarEntry, index)
                    iconName: control.itemIconName(toolbarEntry)
                    iconSource: control.itemIconSource(toolbarEntry)
                    enabled: control.itemEnabled(toolbarEntry)
                    visible: control.itemVisible(toolbarEntry)
                }
            }
        }

        Row {
            id: manualButtons
            visible: !control.usingItemModel
            spacing: control.spacing
        }
    }

    Connections {
        target: modelButtons
        function onChildrenChanged() {
            Qt.callLater(control.normalizeActiveButton)
        }
    }

    Connections {
        target: manualButtons
        function onChildrenChanged() {
            Qt.callLater(control.normalizeActiveButton)
        }
    }

    QtObject {
        Component.onCompleted: {
            control.normalizeActiveButton()
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.HierarchyToolbar {
//     buttonItems: [
//         { id: "structure", iconName: "projectStructure", eventName: "hierarchy.structure" },
//         { id: "layers", iconName: "projectStructure", eventName: "hierarchy.layers" }
//     ]
// }
