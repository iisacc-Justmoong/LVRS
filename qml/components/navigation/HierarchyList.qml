import QtQuick
import LVRS 1.0

Item {
    id: control

    property int rowSpacing: 0
    property var activeItem: null
    property int activeItemId: -1
    property string activeItemKey: ""
    property bool keyboardNavigationEnabled: true

    // Main API: array/list-like model input.
    // Supports nested children arrays or list models via childrenRole.
    property var model: []
    // Backward compatibility alias.
    property alias treeModel: control.model
    property string childrenRole: "children"
    property string itemIdRole: "itemId"
    property string itemKeyRole: "key"
    property string labelRole: "label"
    property string iconNameRole: "iconName"
    property string iconSourceRole: "iconSource"
    property string iconGlyphRole: "iconGlyph"
    property string enabledRole: "enabled"
    property string expandedRole: "expanded"
    property string selectedRole: "selected"
    property string showChevronRole: "showChevron"
    property int autoExpandDepth: 1

    property int generatedIndentStep: 13
    property int generatedRowHeight: 28
    property int generatedItemWidth: 200
    property int generatedIconSize: 16
    property int generatedChevronSize: 16
    property bool autoExpandAncestorsOnActivate: true

    readonly property bool usingTreeModel: modelCount(model) > 0
    readonly property int itemCount: collectItems().length
    readonly property int visibleItemCount: collectVisibleItems(false).length

    signal activeChanged(var item, int itemId, int index)
    signal expansionChanged(var item, bool expanded, int index)
    signal ensureVisibleRequested(real y, real height)

    default property alias items: manualColumn.data

    property var _generatedItems: []

    Component {
        id: generatedItemComponent

        HierarchyItem { }
    }

    function roleValue(node, roleName, fallbackValue) {
        if (!node || typeof node !== "object")
            return fallbackValue
        const key = roleName === undefined || roleName === null ? "" : String(roleName).trim()
        if (key.length === 0)
            return fallbackValue
        if (node[key] !== undefined)
            return node[key]
        return fallbackValue
    }

    function boolRole(node, roleName, fallbackValue) {
        const value = roleValue(node, roleName, undefined)
        if (value === undefined || value === null)
            return !!fallbackValue
        if (typeof value === "boolean")
            return value
        if (typeof value === "number")
            return value !== 0
        if (typeof value === "string") {
            const normalized = value.trim().toLowerCase()
            if (normalized.length === 0 || normalized === "0" || normalized === "false" || normalized === "no")
                return false
            return true
        }
        return !!value
    }

    function modelCount(model) {
        if (model === undefined || model === null)
            return 0
        if (Array.isArray(model))
            return model.length
        if (model.length !== undefined)
            return Math.max(0, Number(model.length) || 0)
        if (model.count !== undefined)
            return Math.max(0, Number(model.count) || 0)
        return 0
    }

    function modelAt(model, index) {
        if (model === undefined || model === null)
            return null
        if (Array.isArray(model))
            return model[index]
        if (model.get !== undefined)
            return model.get(index)
        return model[index]
    }

    function isManagedItem(item) {
        if (!item || item.__isHierarchyItem !== true)
            return false
        if (usingTreeModel)
            return item.generatedByTreeModel === true
        return item.generatedByTreeModel !== true
    }

    function collectItems() {
        const source = usingTreeModel ? generatedColumn : manualColumn
        const result = []
        for (let i = 0; i < source.children.length; i++) {
            const child = source.children[i]
            if (child && child.__isHierarchyItem === true)
                result.push(child)
        }
        return result
    }

    function indexOfItemInList(currentItems, item) {
        for (let i = 0; i < currentItems.length; i++) {
            if (currentItems[i] === item)
                return i
        }
        return -1
    }

    function resolveByIdInList(currentItems, itemId) {
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (effectiveItemId(item, i) === itemId)
                return item
        }
        return null
    }

    function resolveByKeyInList(currentItems, normalizedKey) {
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (effectiveItemKey(item, i) === normalizedKey)
                return item
        }
        return null
    }

    function firstEnabledItemInList(currentItems) {
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (item && item.enabled)
                return item
        }
        return currentItems.length > 0 ? currentItems[0] : null
    }

    function firstInitiallySelectedItemInList(currentItems) {
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (item && item.selected)
                return item
        }
        return null
    }

    function isItemVisibleInList(currentItems, item, itemIndex) {
        if (!item || !isManagedItem(item))
            return false
        if (itemIndex <= 0)
            return true

        const currentIndent = Math.max(0, item && item.indentLevel !== undefined ? item.indentLevel : 0)
        if (currentIndent === 0)
            return true

        let requiredIndent = currentIndent
        for (let i = itemIndex - 1; i >= 0 && requiredIndent > 0; i--) {
            const candidate = currentItems[i]
            const candidateIndent = Math.max(0, candidate && candidate.indentLevel !== undefined ? candidate.indentLevel : 0)
            if (candidateIndent < requiredIndent) {
                if (candidate.showChevron && !candidate.expanded)
                    return false
                requiredIndent = candidateIndent
            }
        }
        return true
    }

    function expandAncestorsForIndexInList(currentItems, itemIndex) {
        if (!autoExpandAncestorsOnActivate || itemIndex <= 0)
            return

        const item = currentItems[itemIndex]
        if (!item)
            return

        let requiredIndent = Math.max(0, item && item.indentLevel !== undefined ? item.indentLevel : 0)
        for (let i = itemIndex - 1; i >= 0 && requiredIndent > 0; i--) {
            const candidate = currentItems[i]
            const candidateIndent = Math.max(0, candidate && candidate.indentLevel !== undefined ? candidate.indentLevel : 0)
            if (candidateIndent < requiredIndent) {
                if (candidate.showChevron && !candidate.expanded)
                    candidate.expanded = true
                requiredIndent = candidateIndent
            }
        }
    }

    function collectVisibleItems(enabledOnly) {
        const result = []
        const currentItems = collectItems()
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (!item)
                continue
            if (enabledOnly && !item.enabled)
                continue
            if (isItemVisibleInList(currentItems, item, i))
                result.push(item)
        }
        return result
    }

    function indexOfItem(item) {
        return indexOfItemInList(collectItems(), item)
    }

    function isItemVisible(item) {
        const currentItems = collectItems()
        const itemIndex = indexOfItemInList(currentItems, item)
        return isItemVisibleInList(currentItems, item, itemIndex)
    }

    function effectiveItemId(item, index) {
        if (item && item.itemId !== undefined && item.itemId >= 0)
            return item.itemId
        return index
    }

    function effectiveItemKey(item, index) {
        if (!item)
            return ""

        const explicitKey = item.itemKey === undefined || item.itemKey === null ? "" : String(item.itemKey).trim()
        if (explicitKey.length > 0)
            return explicitKey

        const effectiveId = effectiveItemId(item, index)
        if (effectiveId >= 0)
            return String(effectiveId)

        return String(index)
    }

    function resolveById(itemId) {
        return resolveByIdInList(collectItems(), itemId)
    }

    function resolveByKey(itemKey) {
        const normalizedKey = itemKey === undefined || itemKey === null ? "" : String(itemKey).trim()
        if (normalizedKey.length === 0)
            return null

        return resolveByKeyInList(collectItems(), normalizedKey)
    }

    function firstEnabledItem() {
        return firstEnabledItemInList(collectItems())
    }

    function firstInitiallySelectedItem() {
        return firstInitiallySelectedItemInList(collectItems())
    }

    function expandAncestorsForIndex(itemIndex) {
        expandAncestorsForIndexInList(collectItems(), itemIndex)
    }

    function registerItem(item) {
        if (!item || !isManagedItem(item))
            return
        if (item.hierarchyList !== control)
            item.hierarchyList = control
        normalizeActiveItem()
    }

    function notifyExpansionChanged(item) {
        if (!item || !isManagedItem(item))
            return
        const currentItems = collectItems()
        const index = indexOfItemInList(currentItems, item)
        expansionChanged(item, !!item.expanded, index)
        const activeIndex = indexOfItemInList(currentItems, activeItem)
        if (activeItem && !isItemVisibleInList(currentItems, activeItem, activeIndex))
            requestActivate(item)
    }

    function requestActivate(item) {
        if (!item || !item.enabled || !isManagedItem(item))
            return
        if (item.hierarchyList !== control)
            item.hierarchyList = control

        const currentItems = collectItems()
        const index = indexOfItemInList(currentItems, item)
        if (index < 0)
            return

        expandAncestorsForIndexInList(currentItems, index)
        if (!isItemVisibleInList(currentItems, item, index))
            return

        const nextId = effectiveItemId(item, index)
        const nextKey = effectiveItemKey(item, index)
        if (activeItem === item && activeItemId === nextId && activeItemKey === nextKey)
            return

        activeItem = item
        activeItemId = nextId
        activeItemKey = nextKey
        ensureVisibleRequested(item.y, item.height)

        if (keyboardNavigationEnabled && !control.activeFocus)
            control.forceActiveFocus()

        activeChanged(item, activeItemId, index)
    }

    function activateById(itemId) {
        const item = resolveById(itemId)
        if (!item)
            return false
        requestActivate(item)
        return activeItem === item
    }

    function activateByKey(itemKey) {
        const item = resolveByKey(itemKey)
        if (!item)
            return false
        requestActivate(item)
        return activeItem === item
    }

    function normalizeActiveItem() {
        const currentItems = collectItems()
        if (currentItems.length === 0) {
            activeItem = null
            activeItemId = -1
            activeItemKey = ""
            return
        }

        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (item && item.hierarchyList !== control)
                item.hierarchyList = control
        }

        let targetItem = activeItem
        if (activeItemKey.length > 0) {
            const byKey = resolveByKeyInList(currentItems, activeItemKey)
            if (byKey)
                targetItem = byKey
        } else if (activeItemId >= 0) {
            const byId = resolveByIdInList(currentItems, activeItemId)
            if (byId)
                targetItem = byId
        }

        if (!targetItem || indexOfItemInList(currentItems, targetItem) === -1 || !targetItem.enabled)
            targetItem = firstInitiallySelectedItemInList(currentItems)
        if (!targetItem || !targetItem.enabled)
            targetItem = firstEnabledItemInList(currentItems)

        const targetIndex = indexOfItemInList(currentItems, targetItem)
        if (targetIndex >= 0)
            expandAncestorsForIndexInList(currentItems, targetIndex)

        if (targetItem && !isItemVisibleInList(currentItems, targetItem, targetIndex))
            targetItem = firstEnabledItemInList(currentItems)

        const finalIndex = indexOfItemInList(currentItems, targetItem)
        activeItem = targetItem
        activeItemId = targetItem ? effectiveItemId(targetItem, finalIndex) : -1
        activeItemKey = targetItem ? effectiveItemKey(targetItem, finalIndex) : ""

        if (targetItem)
            ensureVisibleRequested(targetItem.y, targetItem.height)
    }

    function flattenTreeNodes(nodes, depth, parentKey, parentPath, sink) {
        const count = modelCount(nodes)
        for (let i = 0; i < count; i++) {
            const node = modelAt(nodes, i)
            if (node === undefined || node === null)
                continue

            const isObjectNode = typeof node === "object"
            const primitiveLabel = isObjectNode ? "" : String(node)
            const labelRaw = isObjectNode
                ? roleValue(node, labelRole,
                            roleValue(node, "text",
                                      roleValue(node, "title",
                                                roleValue(node, "name", ""))))
                : primitiveLabel
            const label = labelRaw === undefined || labelRaw === null ? "" : String(labelRaw)

            let iconName = ""
            let iconSource = ""
            if (isObjectNode) {
                const iconToken = roleValue(node, iconNameRole, roleValue(node, "icon", ""))
                if (typeof iconToken === "object" && iconToken !== null) {
                    const nestedIconNameRaw = iconToken.name === undefined || iconToken.name === null
                        ? ""
                        : String(iconToken.name)
                    const nestedIconSourceRaw = iconToken.source === undefined || iconToken.source === null
                        ? (iconToken.url === undefined || iconToken.url === null
                               ? ""
                               : String(iconToken.url))
                        : String(iconToken.source)
                    iconName = nestedIconNameRaw.trim()
                    iconSource = nestedIconSourceRaw.trim()
                } else {
                    const iconTokenText = iconToken === undefined || iconToken === null
                        ? ""
                        : String(iconToken).trim()
                    if (iconTokenText.startsWith("qrc:") || iconTokenText.startsWith(":/") || iconTokenText.indexOf("://") >= 0)
                        iconSource = iconTokenText
                    else
                        iconName = iconTokenText
                }
                const explicitIconSourceRaw = roleValue(node, iconSourceRole, "")
                const explicitIconSource = explicitIconSourceRaw === undefined || explicitIconSourceRaw === null
                    ? ""
                    : String(explicitIconSourceRaw).trim()
                if (explicitIconSource.length > 0)
                    iconSource = explicitIconSource
            }

            const iconGlyphRaw = isObjectNode ? roleValue(node, iconGlyphRole, "") : ""
            const iconGlyph = iconGlyphRaw === undefined || iconGlyphRaw === null ? "" : String(iconGlyphRaw)

            const rawItemId = isObjectNode ? roleValue(node, itemIdRole, roleValue(node, "id", -1)) : -1
            const numericItemId = Number(rawItemId)
            const itemId = Number.isFinite(numericItemId) ? Math.trunc(numericItemId) : -1

            const explicitKeyRaw = isObjectNode ? roleValue(node, itemKeyRole, "") : ""
            const explicitKey = explicitKeyRaw === undefined || explicitKeyRaw === null
                ? ""
                : String(explicitKeyRaw).trim()
            const fallbackKey = parentKey.length > 0 ? parentKey + "/" + i : String(i)
            const itemKey = explicitKey.length > 0
                ? explicitKey
                : itemId >= 0
                    ? String(itemId)
                    : fallbackKey

            const displayLabel = label.length > 0 ? label : itemKey
            const pathLabel = parentPath.length > 0 ? parentPath + " / " + displayLabel : displayLabel

            const childNodes = isObjectNode
                ? roleValue(node, childrenRole,
                            roleValue(node, "items",
                                      roleValue(node, "nodes", [])))
                : []
            const hasChildren = modelCount(childNodes) > 0

            const explicitChevron = isObjectNode ? roleValue(node, showChevronRole, undefined) : undefined
            const showChevron = explicitChevron === undefined || explicitChevron === null
                ? hasChildren
                : !!explicitChevron

            const rawIndentLevel = isObjectNode
                ? roleValue(node, "indentLevel",
                            roleValue(node, "depth", depth))
                : depth
            const numericIndentLevel = Number(rawIndentLevel)
            const indentLevel = Number.isFinite(numericIndentLevel)
                ? Math.max(0, Math.trunc(numericIndentLevel))
                : Math.max(0, depth)

            const parentItemKeyRaw = isObjectNode ? roleValue(node, "parentKey", parentKey) : parentKey
            const parentItemKey = parentItemKeyRaw === undefined || parentItemKeyRaw === null
                ? ""
                : String(parentItemKeyRaw).trim()

            const expandedDefault = depth < autoExpandDepth
            const expanded = hasChildren ? boolRole(node, expandedRole, expandedDefault) : false
            const selected = boolRole(node, selectedRole, false)
            const enabled = boolRole(node, enabledRole, true)

            sink.push({
                          itemId: itemId,
                          itemKey: itemKey,
                          parentItemKey: parentItemKey,
                          label: displayLabel,
                          iconName: iconName,
                          iconSource: iconSource,
                          iconGlyph: iconGlyph,
                          showChevron: showChevron,
                          expanded: expanded,
                          selected: selected,
                          enabled: enabled,
                          indentLevel: indentLevel,
                          pathLabel: pathLabel,
                          nodeData: node
                      })

            if (hasChildren)
                flattenTreeNodes(childNodes, indentLevel + 1, itemKey, pathLabel, sink)
        }
    }

    function clearGeneratedItems() {
        for (let i = 0; i < _generatedItems.length; i++) {
            const item = _generatedItems[i]
            if (item)
                item.destroy()
        }
        _generatedItems = []
    }

    function rebuildTreeItems() {
        clearGeneratedItems()

        if (!usingTreeModel) {
            Qt.callLater(normalizeActiveItem)
            return
        }

        const flattened = []
        flattenTreeNodes(model, 0, "", "", flattened)

        for (let i = 0; i < flattened.length; i++) {
            const descriptor = flattened[i]
            const item = generatedItemComponent.createObject(generatedColumn, {
                                                                 generatedByTreeModel: true,
                                                                 hierarchyList: control,
                                                                 itemId: descriptor.itemId,
                                                                 itemKey: descriptor.itemKey,
                                                                 parentItemKey: descriptor.parentItemKey,
                                                                 pathLabel: descriptor.pathLabel,
                                                                 nodeData: descriptor.nodeData,
                                                                 label: descriptor.label,
                                                                 iconName: descriptor.iconName,
                                                                 iconSource: descriptor.iconSource,
                                                                 iconGlyph: descriptor.iconGlyph,
                                                                 showChevron: descriptor.showChevron,
                                                                 expanded: descriptor.expanded,
                                                                 selected: descriptor.selected,
                                                                 enabled: descriptor.enabled,
                                                                 indentLevel: descriptor.indentLevel,
                                                                 indentStep: control.generatedIndentStep,
                                                                 rowHeight: control.generatedRowHeight,
                                                                 itemWidth: control.generatedItemWidth,
                                                                 iconSize: control.generatedIconSize,
                                                                 chevronSize: control.generatedChevronSize
                                                             })
            if (item)
                _generatedItems.push(item)
        }

        Qt.callLater(normalizeActiveItem)
    }

    function expandAll() {
        const currentItems = collectItems()
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (item && item.showChevron && !item.expanded)
                item.expanded = true
        }
    }

    function collapseAll(keepRootExpanded) {
        const keepRoot = keepRootExpanded === undefined ? true : !!keepRootExpanded
        const currentItems = collectItems()
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (!item || !item.showChevron)
                continue
            const indent = Math.max(0, item.indentLevel !== undefined ? item.indentLevel : 0)
            item.expanded = keepRoot && indent === 0
        }
        normalizeActiveItem()
    }

    function parentItem(item) {
        const currentItems = collectItems()
        const itemIndex = indexOfItemInList(currentItems, item)
        if (itemIndex <= 0)
            return null

        const currentIndent = Math.max(0, item && item.indentLevel !== undefined ? item.indentLevel : 0)
        for (let i = itemIndex - 1; i >= 0; i--) {
            const candidate = currentItems[i]
            const candidateIndent = Math.max(0, candidate && candidate.indentLevel !== undefined ? candidate.indentLevel : 0)
            if (candidateIndent < currentIndent)
                return candidate
        }
        return null
    }

    function firstChildItem(item) {
        const currentItems = collectItems()
        const itemIndex = indexOfItemInList(currentItems, item)
        if (itemIndex < 0)
            return null

        const currentIndent = Math.max(0, item && item.indentLevel !== undefined ? item.indentLevel : 0)
        for (let i = itemIndex + 1; i < currentItems.length; i++) {
            const candidate = currentItems[i]
            const candidateIndent = Math.max(0, candidate && candidate.indentLevel !== undefined ? candidate.indentLevel : 0)
            if (candidateIndent <= currentIndent)
                break
            if (candidateIndent === currentIndent + 1
                    && candidate.enabled
                    && isItemVisibleInList(currentItems, candidate, i)) {
                return candidate
            }
        }
        return null
    }

    function activateRelativeVisible(step) {
        const visibleItems = collectVisibleItems(true)
        if (visibleItems.length === 0)
            return false

        let currentIndex = visibleItems.indexOf(activeItem)
        if (currentIndex < 0)
            currentIndex = step > 0 ? -1 : visibleItems.length

        const targetIndex = Math.max(0, Math.min(currentIndex + step, visibleItems.length - 1))
        const targetItem = visibleItems[targetIndex]
        if (!targetItem)
            return false

        requestActivate(targetItem)
        return true
    }

    function navigateLeft() {
        if (!activeItem)
            return false

        if (activeItem.showChevron && activeItem.expanded) {
            activeItem.expanded = false
            return true
        }

        const parent = parentItem(activeItem)
        if (parent) {
            requestActivate(parent)
            return true
        }

        return false
    }

    function navigateRight() {
        if (!activeItem)
            return false

        if (activeItem.showChevron && !activeItem.expanded) {
            activeItem.expanded = true
            return true
        }

        const child = firstChildItem(activeItem)
        if (child) {
            requestActivate(child)
            return true
        }

        return false
    }

    onModelChanged: Qt.callLater(rebuildTreeItems)
    onChildrenRoleChanged: Qt.callLater(rebuildTreeItems)
    onItemIdRoleChanged: Qt.callLater(rebuildTreeItems)
    onItemKeyRoleChanged: Qt.callLater(rebuildTreeItems)
    onLabelRoleChanged: Qt.callLater(rebuildTreeItems)
    onIconNameRoleChanged: Qt.callLater(rebuildTreeItems)
    onIconSourceRoleChanged: Qt.callLater(rebuildTreeItems)
    onIconGlyphRoleChanged: Qt.callLater(rebuildTreeItems)
    onEnabledRoleChanged: Qt.callLater(rebuildTreeItems)
    onExpandedRoleChanged: Qt.callLater(rebuildTreeItems)
    onSelectedRoleChanged: Qt.callLater(rebuildTreeItems)
    onShowChevronRoleChanged: Qt.callLater(rebuildTreeItems)
    onAutoExpandDepthChanged: Qt.callLater(rebuildTreeItems)
    onGeneratedIndentStepChanged: Qt.callLater(rebuildTreeItems)
    onGeneratedRowHeightChanged: Qt.callLater(rebuildTreeItems)
    onGeneratedItemWidthChanged: Qt.callLater(rebuildTreeItems)
    onGeneratedIconSizeChanged: Qt.callLater(rebuildTreeItems)
    onGeneratedChevronSizeChanged: Qt.callLater(rebuildTreeItems)
    onActiveItemIdChanged: Qt.callLater(normalizeActiveItem)
    onActiveItemKeyChanged: Qt.callLater(normalizeActiveItem)

    implicitWidth: usingTreeModel ? generatedColumn.implicitWidth : manualColumn.implicitWidth
    implicitHeight: usingTreeModel ? generatedColumn.implicitHeight : manualColumn.implicitHeight

    focus: false
    activeFocusOnTab: keyboardNavigationEnabled

    Keys.onUpPressed: function(event) {
        if (!control.keyboardNavigationEnabled)
            return
        event.accepted = control.activateRelativeVisible(-1)
    }
    Keys.onDownPressed: function(event) {
        if (!control.keyboardNavigationEnabled)
            return
        event.accepted = control.activateRelativeVisible(1)
    }
    Keys.onLeftPressed: function(event) {
        if (!control.keyboardNavigationEnabled)
            return
        event.accepted = control.navigateLeft()
    }
    Keys.onRightPressed: function(event) {
        if (!control.keyboardNavigationEnabled)
            return
        event.accepted = control.navigateRight()
    }

    Column {
        id: listColumn
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        Column {
            id: manualColumn
            width: parent.width
            spacing: control.rowSpacing
            visible: !control.usingTreeModel
        }

        Column {
            id: generatedColumn
            width: parent.width
            spacing: control.rowSpacing
            visible: control.usingTreeModel
        }
    }

    Connections {
        target: manualColumn
        function onChildrenChanged() {
            if (!control.usingTreeModel)
                Qt.callLater(control.normalizeActiveItem)
        }
    }

    Connections {
        target: generatedColumn
        function onChildrenChanged() {
            if (control.usingTreeModel)
                Qt.callLater(control.normalizeActiveItem)
        }
    }

    QtObject {
        Component.onCompleted: {
            if (control.usingTreeModel)
                control.rebuildTreeItems()
            else
                control.normalizeActiveItem()
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.HierarchyList {
//     model: [
//         { key: "world", label: "World", expanded: true,
//           children: [{ key: "camera", label: "Camera" }] }
//     ]
// }
