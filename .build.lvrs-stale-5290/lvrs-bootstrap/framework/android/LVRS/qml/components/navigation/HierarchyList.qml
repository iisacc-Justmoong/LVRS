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
    property var _itemsCache: []
    property bool _itemsDirty: true
    property bool _lookupDirty: true
    property var _idIndexMap: ({})
    property var _keyIndexMap: ({})
    property var _visibilityFlags: []
    property var _visibleItemIndices: []
    property var _visibleEnabledItemIndices: []
    property bool _visibilityCacheInitialized: false
    property bool _rebuildScheduled: false
    property bool _normalizeScheduled: false
    property bool _refreshScheduled: false
    property int _pendingRefreshFrom: -1
    property int _pendingRefreshTo: -1
    property bool _fullRefreshRequested: true
    property bool _applyingActiveState: false
    property bool _isRefreshing: false
    property int _rebuildRevision: 0
    property var _rebuildDescriptors: []
    property int _rebuildDescriptorIndex: 0
    property int _rebuildChunkSize: 240
    property bool _isBuildingGeneratedItems: false

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

    function normalizeFromIndex(indexValue) {
        const numericIndex = Number(indexValue)
        if (!Number.isFinite(numericIndex))
            return 0
        return Math.max(0, Math.trunc(numericIndex))
    }

    function normalizeToIndex(indexValue) {
        const numericIndex = Number(indexValue)
        if (!Number.isFinite(numericIndex))
            return -1
        return Math.max(-1, Math.trunc(numericIndex))
    }

    function markItemsDirty(fullRefreshRequested) {
        _itemsDirty = true
        _lookupDirty = true
        if (fullRefreshRequested === undefined || fullRefreshRequested)
            _fullRefreshRequested = true
    }

    function collectItems() {
        if (_itemsDirty) {
            const source = usingTreeModel ? generatedColumn : manualColumn
            const rebuilt = []
            const children = source.children
            for (let i = 0; i < children.length; i++) {
                const child = children[i]
                if (child && child.__isHierarchyItem === true)
                    rebuilt.push(child)
            }
            _itemsCache = rebuilt
            _itemsDirty = false
        }
        return _itemsCache
    }

    function itemIndentLevel(item) {
        const rawIndent = item && item.indentLevel !== undefined ? item.indentLevel : 0
        const numericIndent = Number(rawIndent)
        if (!Number.isFinite(numericIndent))
            return 0
        return Math.max(0, Math.trunc(numericIndent))
    }

    function indexOfItemInList(currentItems, item) {
        if (!item)
            return -1

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

    function idLookupKey(itemId) {
        if (itemId === undefined)
            return "undefined:"
        if (itemId === null)
            return "null:"
        const typeName = typeof itemId
        return typeName + ":" + String(itemId)
    }

    function rebuildLookupMaps(currentItems) {
        const idMap = ({})
        const keyMap = ({})
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (!item)
                continue

            const resolvedId = effectiveItemId(item, i)
            const resolvedIdKey = idLookupKey(resolvedId)
            if (idMap[resolvedIdKey] === undefined)
                idMap[resolvedIdKey] = i

            const resolvedItemKey = effectiveItemKey(item, i)
            if (resolvedItemKey.length > 0 && keyMap[resolvedItemKey] === undefined)
                keyMap[resolvedItemKey] = i
        }
        _idIndexMap = idMap
        _keyIndexMap = keyMap
        _lookupDirty = false
    }

    function ensureStateUpToDate() {
        if (_isRefreshing)
            return

        if (_itemsDirty || _fullRefreshRequested || _pendingRefreshFrom >= 0 || _refreshScheduled) {
            _refreshScheduled = false
            refreshState()
            return
        }

        if (_lookupDirty)
            rebuildLookupMaps(collectItems())
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

    function computeItemVisibilityInList(currentItems, item, itemIndex) {
        if (!item || !isManagedItem(item))
            return false
        if (itemIndex <= 0)
            return true

        const currentIndent = itemIndentLevel(item)
        if (currentIndent === 0)
            return true

        let requiredIndent = currentIndent
        for (let i = itemIndex - 1; i >= 0 && requiredIndent > 0; i--) {
            const candidate = currentItems[i]
            const candidateIndent = itemIndentLevel(candidate)
            if (candidateIndent < requiredIndent) {
                if (candidate.showChevron && !candidate.expanded)
                    return false
                requiredIndent = candidateIndent
            }
        }
        return true
    }

    function isItemVisibleInList(currentItems, item, itemIndex) {
        if (!item || !isManagedItem(item))
            return false

        if (itemIndex >= 0 && itemIndex < _visibilityFlags.length && currentItems[itemIndex] === item) {
            if (_pendingRefreshFrom < 0)
                return !!_visibilityFlags[itemIndex]

            const pendingTo = _pendingRefreshTo < 0 ? currentItems.length - 1 : _pendingRefreshTo
            if (itemIndex < _pendingRefreshFrom || itemIndex > pendingTo)
                return !!_visibilityFlags[itemIndex]
        }

        return computeItemVisibilityInList(currentItems, item, itemIndex)
    }

    function patchIndexRange(existingIndices, fromIndex, toIndex, replacementIndices) {
        const current = Array.isArray(existingIndices) ? existingIndices : []
        const replacement = Array.isArray(replacementIndices) ? replacementIndices : []
        const result = []
        for (let i = 0; i < current.length; i++) {
            const index = current[i]
            if (index < fromIndex)
                result.push(index)
        }
        for (let i = 0; i < replacement.length; i++)
            result.push(replacement[i])
        for (let i = 0; i < current.length; i++) {
            const index = current[i]
            if (index > toIndex)
                result.push(index)
        }
        return result
    }

    function seedVisibilityState(currentItems, startIndex, visibleByDepth, expandedByDepth) {
        if (startIndex <= 0 || currentItems.length === 0)
            return

        const clampedStart = Math.max(0, Math.min(startIndex, currentItems.length - 1))
        let requiredIndent = itemIndentLevel(currentItems[clampedStart])
        for (let i = clampedStart - 1; i >= 0 && requiredIndent > 0; i--) {
            const candidate = currentItems[i]
            const candidateIndent = itemIndentLevel(candidate)
            if (candidateIndent < requiredIndent) {
                visibleByDepth[candidateIndent] = !!_visibilityFlags[i]
                expandedByDepth[candidateIndent] = !candidate.showChevron || !!candidate.expanded
                requiredIndent = candidateIndent
            }
        }
    }

    function refreshVisibleRange(currentItems, startIndex, endIndex) {
        if (currentItems.length === 0) {
            _visibilityFlags = []
            _visibleItemIndices = []
            _visibleEnabledItemIndices = []
            _visibilityCacheInitialized = false
            _itemCountInternal = 0
            _visibleItemCountInternal = 0
            return
        }

        let fromIndex = Math.max(0, Math.min(startIndex, currentItems.length - 1))
        let toIndex = endIndex < 0 ? currentItems.length - 1 : Math.max(fromIndex, Math.min(endIndex, currentItems.length - 1))

        if (_visibilityFlags.length !== currentItems.length || !_visibilityCacheInitialized) {
            const seededVisibility = new Array(currentItems.length)
            for (let i = 0; i < seededVisibility.length; i++)
                seededVisibility[i] = false
            _visibilityFlags = seededVisibility
            _visibleItemIndices = []
            _visibleEnabledItemIndices = []
            fromIndex = 0
            toIndex = currentItems.length - 1
        }

        const visibleByDepth = []
        const expandedByDepth = []
        seedVisibilityState(currentItems, fromIndex, visibleByDepth, expandedByDepth)

        const rangeVisible = []
        const rangeVisibleEnabled = []

        for (let i = fromIndex; i <= toIndex; i++) {
            const item = currentItems[i]
            if (!item) {
                _visibilityFlags[i] = false
                continue
            }

            if (item.hierarchyList !== control)
                item.hierarchyList = control

            const indent = itemIndentLevel(item)
            if (visibleByDepth.length > indent) {
                visibleByDepth.length = indent
                expandedByDepth.length = indent
            }

            let rowVisible = true
            if (indent > 0 && expandedByDepth.length >= indent) {
                const parentVisible = visibleByDepth[indent - 1]
                const parentExpanded = expandedByDepth[indent - 1]
                if (parentVisible !== undefined && parentExpanded !== undefined)
                    rowVisible = !!parentVisible && !!parentExpanded
            }

            _visibilityFlags[i] = rowVisible
            if (item._rowVisibleInternal !== rowVisible)
                item._rowVisibleInternal = rowVisible

            visibleByDepth[indent] = rowVisible
            expandedByDepth[indent] = !item.showChevron || !!item.expanded

            if (rowVisible) {
                rangeVisible.push(i)
                if (item.enabled)
                    rangeVisibleEnabled.push(i)
            }
        }

        _visibleItemIndices = patchIndexRange(_visibleItemIndices, fromIndex, toIndex, rangeVisible)
        _visibleEnabledItemIndices = patchIndexRange(_visibleEnabledItemIndices, fromIndex, toIndex, rangeVisibleEnabled)
        _visibilityCacheInitialized = true

        if (_itemCountInternal !== currentItems.length)
            _itemCountInternal = currentItems.length
        if (_visibleItemCountInternal !== _visibleItemIndices.length)
            _visibleItemCountInternal = _visibleItemIndices.length
    }

    function collectVisibleItems(enabledOnly) {
        ensureStateUpToDate()
        const currentItems = collectItems()
        const visibleIndices = enabledOnly ? _visibleEnabledItemIndices : _visibleItemIndices
        const result = []
        for (let i = 0; i < visibleIndices.length; i++) {
            const itemIndex = visibleIndices[i]
            if (itemIndex < 0 || itemIndex >= currentItems.length)
                continue
            const item = currentItems[itemIndex]
            if (item)
                result.push(item)
        }
        return result
    }

    function expandAncestorsForIndexInList(currentItems, itemIndex) {
        if (!autoExpandAncestorsOnActivate || itemIndex <= 0)
            return -1

        const item = currentItems[itemIndex]
        if (!item)
            return -1

        let earliestChangedIndex = -1
        let requiredIndent = itemIndentLevel(item)
        for (let i = itemIndex - 1; i >= 0 && requiredIndent > 0; i--) {
            const candidate = currentItems[i]
            const candidateIndent = itemIndentLevel(candidate)
            if (candidateIndent < requiredIndent) {
                if (candidate.showChevron && !candidate.expanded) {
                    candidate.expanded = true
                    if (earliestChangedIndex < 0 || i < earliestChangedIndex)
                        earliestChangedIndex = i
                }
                requiredIndent = candidateIndent
            }
        }
        return earliestChangedIndex
    }

    function descendantRangeEndInList(currentItems, itemIndex) {
        if (itemIndex < 0 || itemIndex >= currentItems.length)
            return itemIndex

        const parentIndent = itemIndentLevel(currentItems[itemIndex])
        let descendantEnd = itemIndex
        for (let i = itemIndex + 1; i < currentItems.length; i++) {
            const candidateIndent = itemIndentLevel(currentItems[i])
            if (candidateIndent <= parentIndent)
                break
            descendantEnd = i
        }
        return descendantEnd
    }

    function scheduleRefreshState(fromIndex, toIndex) {
        const normalizedFrom = normalizeFromIndex(fromIndex)
        const normalizedTo = normalizeToIndex(toIndex)

        if (_pendingRefreshFrom < 0) {
            _pendingRefreshFrom = normalizedFrom
            _pendingRefreshTo = normalizedTo
        } else {
            if (normalizedFrom < _pendingRefreshFrom)
                _pendingRefreshFrom = normalizedFrom
            if (_pendingRefreshTo < 0 || normalizedTo < 0)
                _pendingRefreshTo = -1
            else if (normalizedTo > _pendingRefreshTo)
                _pendingRefreshTo = normalizedTo
        }

        if (_refreshScheduled)
            return
        _refreshScheduled = true
        Qt.callLater(function() {
            _refreshScheduled = false
            control.refreshState()
        })
    }

    function refreshState() {
        if (_isRefreshing)
            return

        _isRefreshing = true
        try {
            const currentItems = collectItems()
            if (_lookupDirty)
                rebuildLookupMaps(currentItems)

            if (currentItems.length === 0) {
                _visibilityFlags = []
                _visibleItemIndices = []
                _visibleEnabledItemIndices = []
                _visibilityCacheInitialized = false
                _itemCountInternal = 0
                _visibleItemCountInternal = 0
                if (activeItem)
                    scheduleNormalizeActiveItem()
                _fullRefreshRequested = false
                _pendingRefreshFrom = -1
                _pendingRefreshTo = -1
                return
            }

            let fromIndex = _pendingRefreshFrom
            let toIndex = _pendingRefreshTo
            _pendingRefreshFrom = -1
            _pendingRefreshTo = -1

            if (_fullRefreshRequested || fromIndex < 0) {
                fromIndex = 0
                toIndex = -1
                _fullRefreshRequested = false
            }

            refreshVisibleRange(currentItems, normalizeFromIndex(fromIndex), normalizeToIndex(toIndex))

            const activeIndex = indexOfItemInList(currentItems, activeItem)
            const activeVisible = activeIndex >= 0 && activeIndex < _visibilityFlags.length
                ? !!_visibilityFlags[activeIndex]
                : false
            if (activeItem && (activeIndex < 0 || !activeItem.enabled || !activeVisible))
                scheduleNormalizeActiveItem()
        } finally {
            _isRefreshing = false
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
        ensureStateUpToDate()
        const currentItems = collectItems()
        const itemIndex = indexOfItemInList(currentItems, item)
        if (itemIndex < 0 || itemIndex >= _visibilityFlags.length)
            return false
        return !!_visibilityFlags[itemIndex]
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
        ensureStateUpToDate()
        const currentItems = collectItems()
        const lookupIndex = _idIndexMap[idLookupKey(itemId)]
        if (lookupIndex === undefined || lookupIndex < 0 || lookupIndex >= currentItems.length)
            return null
        return currentItems[lookupIndex] || null
    }

    function resolveByKey(itemKey) {
        ensureStateUpToDate()
        const normalizedKey = itemKey === undefined || itemKey === null ? "" : String(itemKey).trim()
        if (normalizedKey.length === 0)
            return null

        const currentItems = collectItems()
        const lookupIndex = _keyIndexMap[normalizedKey]
        if (lookupIndex === undefined || lookupIndex < 0 || lookupIndex >= currentItems.length)
            return null
        return currentItems[lookupIndex] || null
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
        if (_isBuildingGeneratedItems && item.generatedByTreeModel === true)
            return
        if (item.hierarchyList !== control)
            item.hierarchyList = control

        const currentItems = collectItems()
        const itemIndex = indexOfItemInList(currentItems, item)
        if (itemIndex >= 0) {
            _lookupDirty = true
            scheduleRefreshState(itemIndex, itemIndex)
        } else {
            markItemsDirty(true)
            scheduleRefreshState()
        }
        scheduleNormalizeActiveItem()
    }

    function notifyExpansionChanged(item) {
        if (!item || !isManagedItem(item))
            return

        const currentItems = collectItems()
        const index = indexOfItemInList(currentItems, item)
        expansionChanged(item, !!item.expanded, index)
        if (index >= 0) {
            const descendantEnd = descendantRangeEndInList(currentItems, index)
            const refreshFrom = index + 1
            if (refreshFrom <= descendantEnd)
                scheduleRefreshState(refreshFrom, descendantEnd)
            else
                scheduleRefreshState(index, index)
        } else {
            scheduleRefreshState()
        }

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

        const expandedAncestorIndex = expandAncestorsForIndexInList(currentItems, index)
        if (expandedAncestorIndex >= 0) {
            const expandedRangeEnd = descendantRangeEndInList(currentItems, expandedAncestorIndex)
            const refreshFrom = expandedAncestorIndex + 1
            if (refreshFrom <= expandedRangeEnd)
                scheduleRefreshState(refreshFrom, expandedRangeEnd)
        }

        if (!isItemVisibleInList(currentItems, item, index))
            return

        const changed = applyActiveState(item, index, true)
        if (changed && keyboardNavigationEnabled && !control.activeFocus)
            control.forceActiveFocus()
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
        ensureStateUpToDate()
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
        _isBuildingGeneratedItems = false
        for (let i = 0; i < _generatedItems.length; i++) {
            const item = _generatedItems[i]
            if (item)
                item.destroy()
        }
        _generatedItems = []
        markItemsDirty(true)
        scheduleRefreshState()
    }

    function scheduleRebuildChunk(revision) {
        Qt.callLater(function() {
            control.buildGeneratedItemChunk(revision)
        })
    }

    function buildGeneratedItemChunk(revision) {
        if (revision !== _rebuildRevision)
            return

        _isBuildingGeneratedItems = true
        const descriptors = _rebuildDescriptors
        if (!Array.isArray(descriptors) || descriptors.length === 0) {
            _isBuildingGeneratedItems = false
            _rebuildDescriptors = []
            _rebuildDescriptorIndex = 0
            scheduleNormalizeActiveItem()
            return
        }

        const startIndex = _rebuildDescriptorIndex
        const endExclusive = Math.min(startIndex + _rebuildChunkSize, descriptors.length)
        for (let i = startIndex; i < endExclusive; i++) {
            const descriptor = descriptors[i]
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

        _rebuildDescriptorIndex = endExclusive
        markItemsDirty(true)
        scheduleRefreshState()

        if (_rebuildDescriptorIndex < descriptors.length) {
            scheduleRebuildChunk(revision)
            return
        }

        _isBuildingGeneratedItems = false
        _rebuildDescriptors = []
        _rebuildDescriptorIndex = 0
        scheduleNormalizeActiveItem()
    }

    function rebuildTreeItems() {
        _rebuildRevision = _rebuildRevision + 1
        const revision = _rebuildRevision
        _rebuildDescriptors = []
        _rebuildDescriptorIndex = 0
        _isBuildingGeneratedItems = false

        clearGeneratedItems()

        if (!usingTreeModel) {
            scheduleNormalizeActiveItem()
            return
        }

        const flattened = []
        flattenTreeNodes(model, 0, "", "", flattened)
        _rebuildDescriptors = flattened
        _rebuildDescriptorIndex = 0
        scheduleRebuildChunk(revision)
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

        const currentIndent = itemIndentLevel(item)
        for (let i = itemIndex - 1; i >= 0; i--) {
            const candidate = currentItems[i]
            const candidateIndent = itemIndentLevel(candidate)
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

        const currentIndent = itemIndentLevel(item)
        for (let i = itemIndex + 1; i < currentItems.length; i++) {
            const candidate = currentItems[i]
            const candidateIndent = itemIndentLevel(candidate)
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

    onUsingTreeModelChanged: {
        markItemsDirty(true)
        scheduleRebuildTreeItems()
        scheduleRefreshState()
    }
    onModelChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onChildrenRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onItemIdRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onItemKeyRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onLabelRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onIconNameRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onIconSourceRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onIconGlyphRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onEnabledRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onExpandedRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onSelectedRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onShowChevronRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onAutoExpandDepthChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onGeneratedIndentStepChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onGeneratedRowHeightChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onGeneratedItemWidthChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onGeneratedIconSizeChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onGeneratedChevronSizeChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }

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
            control.markItemsDirty(true)
            control.scheduleRefreshState()
            if (!control.usingTreeModel)
                control.scheduleNormalizeActiveItem()
        }
    }

    Connections {
        target: generatedColumn
        function onChildrenChanged() {
            control.markItemsDirty(true)
            control.scheduleRefreshState()
            if (control.usingTreeModel)
                control.scheduleNormalizeActiveItem()
        }
    }

    QtObject {
        Component.onCompleted: {
            control.markItemsDirty(true)
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
