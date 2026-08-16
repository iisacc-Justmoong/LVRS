pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0

Item {
    id: control

    property int minimumToolbarWidth: Theme.scaleMetric(200)
    property int horizontalPadding: Theme.gap8
    property int verticalPadding: Theme.gap2
    property int spacing: Theme.gapNone
    property color backgroundColor: Theme.subSurface
    property real backgroundOpacity: 0.0
    property int slotSize: Theme.controlHeightSm
    property bool distributeSpacing: false

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
    readonly property int visibleButtonCount: {
        if (usingItemModel)
            return visibleModelButtonCount()
        return countVisibleButtonsInList(collectButtonsInRow(manualButtons))
    }
    readonly property real distributedSpacing: {
        if (!distributeSpacing)
            return spacing
        const count = visibleButtonCount
        if (count < 2)
            return 0
        const availableWidth = Math.max(0, rowContainer.width)
        const consumedWidth = usingItemModel
            ? count * slotSize
            : sumVisibleButtonWidthsInList(collectButtonsInRow(manualButtons))
        return Math.max(0, (availableWidth - consumedWidth) / (count - 1))
    }

    property var activeButton: null
    property var activeButtonId: -1
    property int activeIndex: -1
    property bool _normalizeScheduled: false
    property var _manualClickBindings: []

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
        return collectButtonsInRow(sourceRow)
    }

    function visibleModelButtonCount() {
        let count = 0
        for (let index = 0; index < itemCount; index++) {
            if (itemVisible(itemAt(index)))
                count += 1
        }
        return count
    }

    function visibleModelIndex(modelIndex) {
        let visibleIndex = 0
        for (let index = 0; index < modelIndex; index++) {
            if (itemVisible(itemAt(index)))
                visibleIndex += 1
        }
        return visibleIndex
    }

    function modelButtonX(modelIndex) {
        if (!itemVisible(itemAt(modelIndex)))
            return 0
        const step = slotSize + distributedSpacing
        return visibleModelIndex(modelIndex) * step
    }

    function isToolbarCandidate(child) {
        if (!child)
            return false
        if (child.__isToolbarButton === true)
            return true
        return child.clicked !== undefined
            && child.enabled !== undefined
            && child.visible !== undefined
            && child.tone !== undefined
    }

    function collectButtonsInRow(sourceRow) {
        const result = []
        if (!sourceRow)
            return result
        for (let i = 0; i < sourceRow.children.length; i++) {
            const child = sourceRow.children[i]
            if (isToolbarCandidate(child))
                result.push(child)
        }
        return result
    }

    function countVisibleButtonsInList(buttons) {
        let count = 0
        for (let i = 0; i < buttons.length; i++) {
            const button = buttons[i]
            if (button && button.visible)
                count += 1
        }
        return count
    }

    function sumVisibleButtonWidthsInList(buttons) {
        let widthSum = 0
        for (let i = 0; i < buttons.length; i++) {
            const button = buttons[i]
            if (!button || !button.visible)
                continue
            const widthValue = Number(button.width)
            if (widthValue > 0)
                widthSum += widthValue
            else if (button.implicitWidth > 0)
                widthSum += button.implicitWidth
        }
        return widthSum
    }

    function buttonResolvedId(button, index) {
        if (!button)
            return index
        if (button.buttonId !== undefined && button.buttonId !== null)
            return button.buttonId
        if (button.objectName !== undefined && button.objectName !== null) {
            const normalizedObjectName = String(button.objectName).trim()
            if (normalizedObjectName.length > 0)
                return normalizedObjectName
        }
        return index
    }

    function applyManualButtonToneState(buttons) {
        if (usingItemModel)
            return
        for (let i = 0; i < buttons.length; i++) {
            const button = buttons[i]
            if (!button || button.__isToolbarButton === true || button.tone === undefined)
                continue
            button.tone = (button === activeButton)
                ? AbstractButton.Default
                : AbstractButton.Borderless
        }
    }

    function syncManualClickBindings() {
        if (usingItemModel) {
            for (let i = 0; i < _manualClickBindings.length; i++) {
                const wired = _manualClickBindings[i]
                if (!wired || !wired.button || wired.button.clicked === undefined)
                    continue
                try {
                    wired.button.clicked.disconnect(wired.handler)
                } catch (error) {}
            }
            _manualClickBindings = []
            return
        }

        const buttons = collectButtonsInRow(manualButtons)

        for (let i = _manualClickBindings.length - 1; i >= 0; i--) {
            const wired = _manualClickBindings[i]
            if (!wired || !wired.button || indexOfButtonInList(buttons, wired.button) !== -1)
                continue
            if (wired.button.clicked !== undefined) {
                try {
                    wired.button.clicked.disconnect(wired.handler)
                } catch (error) {}
            }
            _manualClickBindings.splice(i, 1)
        }

        for (let i = 0; i < buttons.length; i++) {
            const button = buttons[i]
            if (!button || button.__isToolbarButton === true || button.clicked === undefined)
                continue

            let alreadyWired = false
            for (let bindingIndex = 0; bindingIndex < _manualClickBindings.length; bindingIndex++) {
                if (_manualClickBindings[bindingIndex].button === button) {
                    alreadyWired = true
                    break
                }
            }
            if (alreadyWired)
                continue

            const handler = (function(targetButton) {
                return function() {
                    control.requestActivate(targetButton)
                }
            })(button)

            button.clicked.connect(handler)
            _manualClickBindings.push({
                button: button,
                handler: handler
            })
        }
    }

    function indexOfButtonInList(buttons, button) {
        for (let i = 0; i < buttons.length; i++) {
            if (buttons[i] === button)
                return i
        }
        return -1
    }

    function resolveByIdInList(buttons, buttonId) {
        for (let i = 0; i < buttons.length; i++) {
            const button = buttons[i]
            if (button && buttonResolvedId(button, i) === buttonId)
                return button
        }
        return null
    }

    function firstSelectedButtonInList(buttons) {
        if (!usingItemModel)
            return null
        for (let i = 0; i < buttons.length; i++) {
            const button = buttons[i]
            const entry = itemAt(i)
            if (button && button.enabled && itemSelected(entry))
                return button
        }
        return null
    }

    function firstEnabledButtonInList(buttons) {
        for (let i = 0; i < buttons.length; i++) {
            const button = buttons[i]
            if (button && button.enabled)
                return button
        }
        return buttons.length > 0 ? buttons[0] : null
    }

    function buttonAt(index) {
        const buttons = collectButtons()
        if (index < 0 || index >= buttons.length)
            return null
        return buttons[index]
    }

    function indexOfButton(button) {
        return indexOfButtonInList(collectButtons(), button)
    }

    function resolveById(buttonId) {
        return resolveByIdInList(collectButtons(), buttonId)
    }

    function firstSelectedButton() {
        return firstSelectedButtonInList(collectButtons())
    }

    function firstEnabledButton() {
        return firstEnabledButtonInList(collectButtons())
    }

    function registerButton(button) {
        if (!button)
            return
        if (button.__isToolbarButton === true && button.toolbar !== control)
            button.toolbar = control
        syncManualClickBindings()
        scheduleNormalizeActiveButton()
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
        const buttonId = button ? buttonResolvedId(button, index) : index
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
        if (button.__isToolbarButton === true && button.toolbar !== control)
            button.toolbar = control

        const buttons = collectButtons()
        const index = indexOfButtonInList(buttons, button)
        if (index < 0)
            return

        const changed = activeButton !== button
        activeButton = button
        activeButtonId = buttonResolvedId(button, index)
        activeIndex = index
        applyManualButtonToneState(buttons)
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
            if (button && button.__isToolbarButton === true && button.toolbar !== control)
                button.toolbar = control
        }

        let targetButton = activeButton
        if (activeButtonId !== undefined && activeButtonId !== null) {
            const byId = resolveByIdInList(buttons, activeButtonId)
            if (byId)
                targetButton = byId
        }
        if (!targetButton || indexOfButtonInList(buttons, targetButton) === -1 || !targetButton.enabled)
            targetButton = firstSelectedButtonInList(buttons)
        if (!targetButton || indexOfButtonInList(buttons, targetButton) === -1 || !targetButton.enabled)
            targetButton = firstEnabledButtonInList(buttons)

        activeButton = targetButton
        activeIndex = targetButton ? indexOfButtonInList(buttons, targetButton) : -1
        activeButtonId = targetButton ? buttonResolvedId(targetButton, activeIndex) : -1
        applyManualButtonToneState(buttons)
    }

    function scheduleNormalizeActiveButton() {
        if (_normalizeScheduled)
            return
        _normalizeScheduled = true
        Qt.callLater(function() {
            _normalizeScheduled = false
            normalizeActiveButton()
        })
    }

    function triggerIndex(index) {
        const button = buttonAt(index)
        if (!button || !button.enabled)
            return false
        requestActivate(button)
        return true
    }

    onActiveButtonIdChanged: scheduleNormalizeActiveButton()
    onButtonItemsChanged: {
        syncManualClickBindings()
        scheduleNormalizeActiveButton()
    }
    onUsingItemModelChanged: {
        syncManualClickBindings()
        scheduleNormalizeActiveButton()
    }

    implicitWidth: {
        if (usingItemModel) {
            const count = buttonCount
            const distributed = distributeSpacing
                ? 0
                : Math.max(0, count - 1) * spacing
            const contentWidth = (count * slotSize) + distributed + (horizontalPadding * 2)
            return Math.max(minimumToolbarWidth, contentWidth)
        }
        return Math.max(minimumToolbarWidth,
                        manualButtons.implicitWidth + (horizontalPadding * 2))
    }
    implicitHeight: {
        const contentHeight = usingItemModel
            ? slotSize
            : Math.max(slotSize, manualButtons.implicitHeight)
        return contentHeight + (verticalPadding * 2)
    }

    Rectangle {
        anchors.fill: parent
        color: control.backgroundColor
        opacity: control.backgroundOpacity
    }

    Item {
        id: rowContainer
        anchors.fill: parent
        anchors.leftMargin: control.horizontalPadding
        anchors.rightMargin: control.horizontalPadding
        anchors.topMargin: control.verticalPadding
        anchors.bottomMargin: control.verticalPadding
        clip: true

        Item {
            id: modelButtons
            visible: control.usingItemModel
            anchors.fill: parent

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
                    width: control.slotSize
                    height: control.slotSize
                    x: control.modelButtonX(index)
                    y: Math.max(0, Math.round((modelButtons.height - height) * 0.5))
                }
            }
        }

        Row {
            id: manualButtons
            visible: !control.usingItemModel
            anchors.fill: parent
            spacing: control.distributedSpacing
        }
    }

    Connections {
        target: modelButtons
        function onChildrenChanged() {
            control.scheduleNormalizeActiveButton()
        }
    }

    Connections {
        target: manualButtons
        function onChildrenChanged() {
            control.syncManualClickBindings()
            control.scheduleNormalizeActiveButton()
        }
    }

    QtObject {
        Component.onCompleted: {
            control.syncManualClickBindings()
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
