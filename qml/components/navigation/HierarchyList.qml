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
    property int _itemCountInternal: 0
    property int _visibleItemCountInternal: 0
    readonly property int itemCount: _itemCountInternal
    readonly property int visibleItemCount: _visibleItemCountInternal

    signal activeChanged(var item, int itemId, int index)
    signal expansionChanged(var item, bool expanded, int index)
    signal ensureVisibleRequested(real y, real height)

    default property alias items: manualColumn.data

    property var _generatedItems: []
    property bool _rebuildScheduled: false
    property bool _normalizeScheduled: false
    property bool _refreshScheduled: false
    property bool _applyingActiveState: false

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

    function modelCount(modelData) {
        if (modelData === undefined || modelData === null)
            return 0
        if (Array.isArray(modelData))
            return modelData.length
        if (modelData.length !== undefined)
            return Math.max(0, Number(modelData.length) || 0)
        if (modelData.count !== undefined)
            return Math.max(0, Number(modelData.count) || 0)
        return 0
    }

    function modelAt(modelData, index) {
        if (modelData === undefined || modelData === null)
            return null
        if (Array.isArray(modelData))
            return modelData[index]
        if (modelData.get !== undefined)
            return modelData.get(index)
        return modelData[index]
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
        const children = source.children
        for (let i = 0; i < children.length; i++) {
            const child = children[i]
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

    function scheduleRefreshState() {
        if (_refreshScheduled)
            return
        _refreshScheduled = true
        Qt.callLater(function() {
            _refreshScheduled = false
            control.refreshState()
        })
    }

    function refreshState() {
        const currentItems = collectItems()
        let visibleCount = 0

        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (!item)
                continue

            if (item.hierarchyList !== control)
                item.hierarchyList = control

            const isVisible = isItemVisibleInList(currentItems, item, i)
            if (item._rowVisibleInternal !== isVisible)
                item._rowVisibleInternal = isVisible
            if (isVisible)
                visibleCount++
        }

        if (_itemCountInternal !== currentItems.length)
            _itemCountInternal = currentItems.length
        if (_visibleItemCountInternal !== visibleCount)
            _visibleItemCountInternal = visibleCount

        const activeIndex = indexOfItemInList(currentItems, activeItem)
        if (activeItem
                && (activeIndex < 0
                    || !activeItem.enabled
                    || !isItemVisibleInList(currentItems, activeItem, activeIndex))) {
            scheduleNormalizeActiveItem()
        }
    }

    function scheduleRebuildTreeItems() {
        if (_rebuildScheduled)
            return
        _rebuildScheduled = true
        Qt.callLater(function() {
            _rebuildScheduled = false
            control.rebuildTreeItems()
        })
    }

    function scheduleNormalizeActiveItem() {
        if (_normalizeScheduled)
            return
        _normalizeScheduled = true
        Qt.callLater(function() {
            _normalizeScheduled = false
            control.normalizeActiveItem()
        })
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
        scheduleRefreshState()
        scheduleNormalizeActiveItem()
    }

    function notifyExpansionChanged(item) {
        if (!item || !isManagedItem(item))
            return

        const currentItems = collectItems()
        const index = indexOfItemInList(currentItems, item)
        expansionChanged(item, !!item.expanded, index)
        scheduleRefreshState()

        const activeIndex = indexOfItemInList(currentItems, activeItem)
        if (activeItem && !isItemVisibleInList(currentItems, activeItem, activeIndex))
            requestActivate(item)
    }

    function applyActiveState(item, index, emitSignal) {
        const nextItem = item || null
        const nextIndex = nextItem ? index : -1
        const nextId = nextItem ? effectiveItemId(nextItem, nextIndex) : -1
        const nextKey = nextItem ? effectiveItemKey(nextItem, nextIndex) : ""

        const changed = activeItem !== nextItem || activeItemId !== nextId || activeItemKey !== nextKey
        if (!changed)
            return false

        _applyingActiveState = true
        activeItem = nextItem
        activeItemId = nextId
        activeItemKey = nextKey
        _applyingActiveState = false

        if (nextItem)
            ensureVisibleRequested(nextItem.y, nextItem.height)

        if (emitSignal)
            activeChanged(nextItem, nextId, nextIndex)

        return true
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

        const changed = applyActiveState(item, index, true)
        if (changed && keyboardNavigationEnabled && !control.activeFocus)
            control.forceActiveFocus()

        scheduleRefreshState()
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
            applyActiveState(null, -1, false)
            scheduleRefreshState()
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

        let targetIndex = indexOfItemInList(currentItems, targetItem)
        if (targetIndex >= 0)
            expandAncestorsForIndexInList(currentItems, targetIndex)

        if (targetItem && !isItemVisibleInList(currentItems, targetItem, targetIndex)) {
            targetItem = firstEnabledItemInList(currentItems)
            targetIndex = indexOfItemInList(currentItems, targetItem)
        }

        applyActiveState(targetItem, targetIndex, false)
        scheduleRefreshState()
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
        scheduleRefreshState()
    }

    function rebuildTreeItems() {
        clearGeneratedItems()

        if (!usingTreeModel) {
            scheduleNormalizeActiveItem()
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

        scheduleRefreshState()
        scheduleNormalizeActiveItem()
    }

    function expandAll() {
        const currentItems = collectItems()
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (item && item.showChevron && !item.expanded)
                item.expanded = true
        }
        scheduleRefreshState()
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
        scheduleRefreshState()
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
            scheduleRefreshState()
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
            scheduleRefreshState()
            return true
        }

        const child = firstChildItem(activeItem)
        if (child) {
            requestActivate(child)
            return true
        }

        return false
    }

    onUsingTreeModelChanged: {
        scheduleRebuildTreeItems()
        scheduleRefreshState()
    }
    onModelChanged: scheduleRebuildTreeItems()
    onChildrenRoleChanged: scheduleRebuildTreeItems()
    onItemIdRoleChanged: scheduleRebuildTreeItems()
    onItemKeyRoleChanged: scheduleRebuildTreeItems()
    onLabelRoleChanged: scheduleRebuildTreeItems()
    onIconNameRoleChanged: scheduleRebuildTreeItems()
    onIconSourceRoleChanged: scheduleRebuildTreeItems()
    onIconGlyphRoleChanged: scheduleRebuildTreeItems()
    onEnabledRoleChanged: scheduleRebuildTreeItems()
    onExpandedRoleChanged: scheduleRebuildTreeItems()
    onSelectedRoleChanged: scheduleRebuildTreeItems()
    onShowChevronRoleChanged: scheduleRebuildTreeItems()
    onAutoExpandDepthChanged: scheduleRebuildTreeItems()
    onGeneratedIndentStepChanged: scheduleRebuildTreeItems()
    onGeneratedRowHeightChanged: scheduleRebuildTreeItems()
    onGeneratedItemWidthChanged: scheduleRebuildTreeItems()
    onGeneratedIconSizeChanged: scheduleRebuildTreeItems()
    onGeneratedChevronSizeChanged: scheduleRebuildTreeItems()

    onActiveItemChanged: {
        if (!_applyingActiveState)
            scheduleNormalizeActiveItem()
    }
    onActiveItemIdChanged: {
        if (!_applyingActiveState)
            scheduleNormalizeActiveItem()
    }
    onActiveItemKeyChanged: {
        if (!_applyingActiveState)
            scheduleNormalizeActiveItem()
    }

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
            control.scheduleRefreshState()
            if (!control.usingTreeModel)
                control.scheduleNormalizeActiveItem()
        }
    }

    Connections {
        target: generatedColumn
        function onChildrenChanged() {
            control.scheduleRefreshState()
            if (control.usingTreeModel)
                control.scheduleNormalizeActiveItem()
        }
    }

    QtObject {
        Component.onCompleted: {
            if (control.usingTreeModel)
                control.rebuildTreeItems()
            else
                control.normalizeActiveItem()
            control.scheduleRefreshState()
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
