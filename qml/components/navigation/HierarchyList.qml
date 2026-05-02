import QtQuick
import LVRS 1.0

Item {
    id: control

    property int rowSpacing: 0
    property var activeItem: null
    property int activeItemId: -1
    property string activeItemKey: ""
    property bool keyboardNavigationEnabled: true
    property bool editable: false

    // Main API: array/list-like depth model input.
    // Each row should provide explicit depth data through `indentLevel` or `depthRole`.
    property var model: []
    // Backward compatibility alias.
    property alias treeModel: control.model
    property int modelColumn: 0
    property string itemIdRole: "itemId"
    property string itemKeyRole: "key"
    property string labelRole: "label"
    property string iconNameRole: "iconName"
    property string iconSourceRole: "iconSource"
    property string iconGlyphRole: "iconGlyph"
    property string countRole: "count"
    property string enabledRole: "enabled"
    property string expandedRole: "expanded"
    property string selectedRole: "selected"
    property string activatableRole: "activatable"
    property string draggableRole: "draggable"
    property string showChevronRole: "showChevron"
    property string depthRole: "depth"

    property int generatedIndentStep: Theme.scaleMetric(8)
    property int generatedRowHeight: Theme.scaleMetric(20)
    property int generatedItemWidth: Theme.scaleMetric(200)
    property int generatedIconSize: Theme.scaleMetric(16)
    property int generatedChevronSize: Theme.scaleMetric(16)
    property bool autoExpandAncestorsOnActivate: true
    readonly property bool editableSupported: hierarchyModel.revision >= 0
        && (hierarchyModel.sourceSupportsEditing() || qmlListModelSupportsEditing())
    readonly property bool editableEnabled: editable && editableSupported

    readonly property bool usingTreeModel: hierarchyModel.count > 0
    property int _itemCountInternal: 0
    property int _visibleItemCountInternal: 0
    readonly property int itemCount: _itemCountInternal
    readonly property int visibleItemCount: _visibleItemCountInternal

    signal activeChanged(var item, int itemId, int index)
    signal expansionChanged(var item, bool expanded, int index)
    signal ensureVisibleRequested(real y, real height)
    signal itemMoved(var item, int itemId, string itemKey, int fromIndex, int toIndex, int depth)

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
    property bool _dragActiveInternal: false
    property var _dragItem: null
    property int _dragSourceIndex: -1
    property int _dragSourceEndIndex: -1
    property int _dragTargetIndex: -1
    property int _dragTargetDepth: -1
    property string _dragTargetModeName: ""
    property var _dragTargetParentItem: null
    property string _dragTargetParentItemKey: ""
    property string _dragTargetParentLabel: ""
    property string _dragTargetParentPathLabel: ""
    property var _dragTargetAnchorItem: null
    property string _dragTargetAnchorItemKey: ""
    property string _dragTargetAnchorLabel: ""
    property int _dragRawInsertionIndex: -1
    property real _dragPointerX: 0
    property real _dragPointerY: 0
    property real _dragIndicatorY: 0
    property real _dragIndicatorX: 0

    Component {
        id: generatedItemComponent

        HierarchyItem { }
    }

    HierarchyModel {
        id: hierarchyModel
        source: control.model
        column: control.modelColumn
        itemIdRole: control.itemIdRole
        itemKeyRole: control.itemKeyRole
        labelRole: control.labelRole
        iconNameRole: control.iconNameRole
        iconSourceRole: control.iconSourceRole
        iconGlyphRole: control.iconGlyphRole
        countRole: control.countRole
        enabledRole: control.enabledRole
        expandedRole: control.expandedRole
        selectedRole: control.selectedRole
        activatableRole: control.activatableRole
        draggableRole: control.draggableRole
        showChevronRole: control.showChevronRole
        depthRole: control.depthRole
    }

    function roleValue(node, roleName, fallbackValue) {
        return hierarchyModel.roleValue(node, roleName, fallbackValue)
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

    function normalizedDepth(rawDepth, fallbackDepth) {
        const numericDepth = Number(rawDepth)
        if (Number.isFinite(numericDepth))
            return Math.max(0, Math.trunc(numericDepth))
        return Math.max(0, Math.trunc(fallbackDepth))
    }

    function resolvedIndentLevel(node, fallbackDepth) {
        if (!node || typeof node !== "object")
            return normalizedDepth(fallbackDepth, 0)

        const explicitIndentLevel = roleValue(node, "indentLevel", undefined)
        if (explicitIndentLevel !== undefined && explicitIndentLevel !== null)
            return normalizedDepth(explicitIndentLevel, fallbackDepth)

        const explicitDepth = roleValue(node, depthRole, undefined)
        if (explicitDepth !== undefined && explicitDepth !== null)
            return normalizedDepth(explicitDepth, fallbackDepth)

        return normalizedDepth(fallbackDepth, 0)
    }

    function modelCount(modelData) {
        if (modelData === model)
            return hierarchyModel.count
        return 0
    }

    function modelAt(modelData, index) {
        if (modelData === model)
            return hierarchyModel.descriptorAt(index).nodeData
        return null
    }

    function invalidateModel() {
        if (_dragActiveInternal)
            _endEditableDrag(false)
        markItemsDirty(true)
        scheduleRebuildTreeItems()
        scheduleRefreshState()
    }

    function normalizedRoleName(roleName, fallbackValue) {
        const rawRoleName = roleName === undefined || roleName === null ? "" : String(roleName).trim()
        if (rawRoleName.length > 0)
            return rawRoleName
        return fallbackValue === undefined || fallbackValue === null ? "" : String(fallbackValue)
    }

    function normalizedTextValue(value) {
        if (value === undefined || value === null)
            return ""
        return String(value)
    }

    function depthArraySupportsEditing(nodes) {
        return hierarchyModel.depthArraySupportsEditing(nodes)
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

    function itemCanBecomeActive(item) {
        if (!item)
            return false

        const activatable = item.activatable === undefined ? true : !!item.activatable
        return !!item.enabled && activatable
    }

    function itemHasChildrenAtIndexInList(currentItems, itemIndex) {
        if (itemIndex < 0 || itemIndex >= currentItems.length - 1)
            return false

        const item = currentItems[itemIndex]
        const nextItem = currentItems[itemIndex + 1]
        if (!item || !nextItem)
            return false

        return itemIndentLevel(nextItem) > itemIndentLevel(item)
    }

    function itemCanExpandInList(currentItems, item, itemIndex) {
        if (!item || !item.showChevron)
            return false
        return itemHasChildrenAtIndexInList(currentItems, itemIndex)
    }

    function syncItemChildPresence(currentItems) {
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (!item || item.hasChildItems === undefined)
                continue

            const hasChildren = itemHasChildrenAtIndexInList(currentItems, i)
            if (item.hasChildItems !== hasChildren)
                item.hasChildItems = hasChildren
        }
    }

    function itemDisplayLabel(item, index) {
        if (!item)
            return ""

        const explicitLabel = normalizedTextValue(item.label === undefined ? item.text : item.label)
        if (explicitLabel.length > 0)
            return explicitLabel
        return effectiveItemKey(item, index)
    }

    function arrayTextEquals(left, right) {
        const leftArray = Array.isArray(left) ? left : []
        const rightArray = Array.isArray(right) ? right : []
        if (leftArray.length !== rightArray.length)
            return false

        for (let i = 0; i < leftArray.length; i++) {
            if (String(leftArray[i]) !== String(rightArray[i]))
                return false
        }
        return true
    }

    function modelList(value) {
        if (Array.isArray(value))
            return value.slice()
        const result = []
        if (!value)
            return result
        if (typeof value === "string")
            return value.length > 0 ? [value] : result
        if (value instanceof String) {
            const textValue = String(value)
            return textValue.length > 0 ? [textValue] : result
        }
        if (value.charAt !== undefined && value.length !== undefined) {
            const textValue = String(value)
            return textValue.length > 0 ? [textValue] : result
        }
        if (value.length !== undefined) {
            for (let i = 0; i < value.length; i++)
                result.push(value[i])
            return result
        }
        if (value.count !== undefined && value.get !== undefined) {
            for (let i = 0; i < value.count; i++)
                result.push(value.get(i))
        }
        return result
    }

    function syncItemExposedMetadata(currentItems) {
        const itemTotal = currentItems.length
        if (itemTotal === 0)
            return

        const childCounts = new Array(itemTotal)
        const visibleChildCounts = new Array(itemTotal)
        const descendantCounts = new Array(itemTotal)
        const visibleDescendantCounts = new Array(itemTotal)
        const parentIndices = new Array(itemTotal)
        const parentLabels = new Array(itemTotal)
        const parentPathLabels = new Array(itemTotal)
        const pathLabels = new Array(itemTotal)
        const ancestorKeys = new Array(itemTotal)
        const ancestorLabels = new Array(itemTotal)
        const pathKeys = new Array(itemTotal)
        const pathLabelArrays = new Array(itemTotal)
        const visibleIndices = new Array(itemTotal)
        const siblingIndices = new Array(itemTotal)
        const visibleSiblingIndices = new Array(itemTotal)
        const siblingCounts = new Array(itemTotal)
        const visibleSiblingCounts = new Array(itemTotal)
        const childKeys = new Array(itemTotal)
        const childLabels = new Array(itemTotal)
        const ancestorStack = []
        const nextSiblingByDepth = []
        const nextVisibleSiblingByDepth = []
        const siblingCountByParent = ({})
        const visibleSiblingCountByParent = ({})
        let visibleIndexCounter = 0

        for (let i = 0; i < itemTotal; i++) {
            childCounts[i] = 0
            visibleChildCounts[i] = 0
            descendantCounts[i] = 0
            visibleDescendantCounts[i] = 0
            parentIndices[i] = -1
            parentLabels[i] = ""
            parentPathLabels[i] = ""
            pathLabels[i] = ""
            ancestorKeys[i] = []
            ancestorLabels[i] = []
            pathKeys[i] = []
            pathLabelArrays[i] = []
            visibleIndices[i] = -1
            siblingIndices[i] = -1
            visibleSiblingIndices[i] = -1
            siblingCounts[i] = 0
            visibleSiblingCounts[i] = 0
            childKeys[i] = []
            childLabels[i] = []
        }

        for (let i = 0; i < itemTotal; i++) {
            const item = currentItems[i]
            if (!item)
                continue

            const indent = itemIndentLevel(item)
            if (ancestorStack.length > indent)
                ancestorStack.length = indent
            if (nextSiblingByDepth.length > indent + 1)
                nextSiblingByDepth.length = indent + 1
            if (nextVisibleSiblingByDepth.length > indent + 1)
                nextVisibleSiblingByDepth.length = indent + 1

            const parentIndex = indent > 0 && ancestorStack.length >= indent
                ? ancestorStack[indent - 1]
                : -1
            parentIndices[i] = parentIndex
            parentLabels[i] = parentIndex >= 0
                ? itemDisplayLabel(currentItems[parentIndex], parentIndex)
                : ""
            parentPathLabels[i] = parentIndex >= 0 ? pathLabels[parentIndex] : ""

            const displayLabel = itemDisplayLabel(item, i)
            const parentPath = parentPathLabels[i]
            pathLabels[i] = parentPath.length > 0 ? parentPath + " / " + displayLabel : displayLabel
            ancestorKeys[i] = parentIndex >= 0 ? pathKeys[parentIndex].slice() : []
            ancestorLabels[i] = parentIndex >= 0 ? pathLabelArrays[parentIndex].slice() : []
            pathKeys[i] = ancestorKeys[i].concat([effectiveItemKey(item, i)])
            pathLabelArrays[i] = ancestorLabels[i].concat([displayLabel])

            const parentLookupKey = String(parentIndex)
            siblingCountByParent[parentLookupKey] = (siblingCountByParent[parentLookupKey] || 0) + 1

            const nextSiblingIndex = nextSiblingByDepth[indent] === undefined
                ? 0
                : nextSiblingByDepth[indent]
            siblingIndices[i] = nextSiblingIndex
            nextSiblingByDepth[indent] = nextSiblingIndex + 1

            if (_visibilityFlags[i]) {
                visibleIndices[i] = visibleIndexCounter
                visibleIndexCounter += 1
                visibleSiblingCountByParent[parentLookupKey] = (visibleSiblingCountByParent[parentLookupKey] || 0) + 1
                const nextVisibleSiblingIndex = nextVisibleSiblingByDepth[indent] === undefined
                    ? 0
                    : nextVisibleSiblingByDepth[indent]
                visibleSiblingIndices[i] = nextVisibleSiblingIndex
                nextVisibleSiblingByDepth[indent] = nextVisibleSiblingIndex + 1
            }

            for (let ancestor = 0; ancestor < ancestorStack.length; ancestor++) {
                descendantCounts[ancestorStack[ancestor]] += 1
                if (_visibilityFlags[i])
                    visibleDescendantCounts[ancestorStack[ancestor]] += 1
            }

            if (parentIndex >= 0) {
                childCounts[parentIndex] += 1
                childKeys[parentIndex].push(effectiveItemKey(item, i))
                childLabels[parentIndex].push(displayLabel)
                if (_visibilityFlags[i])
                    visibleChildCounts[parentIndex] += 1
            }

            ancestorStack[indent] = i
            if (ancestorStack.length > indent + 1)
                ancestorStack.length = indent + 1
        }

        for (let i = 0; i < itemTotal; i++) {
            const item = currentItems[i]
            if (!item)
                continue

            const parentIndex = parentIndices[i]
            const resolvedParentKey = parentIndex >= 0
                ? effectiveItemKey(currentItems[parentIndex], parentIndex)
                : ""
            const siblingCount = siblingCountByParent[String(parentIndex)] || 0
            const visibleSiblingCount = visibleSiblingCountByParent[String(parentIndex)] || 0

            if (item.parentItemKey !== resolvedParentKey)
                item.parentItemKey = resolvedParentKey
            if (item.parentLabel !== parentLabels[i])
                item.parentLabel = parentLabels[i]
            if (item.parentPathLabel !== parentPathLabels[i])
                item.parentPathLabel = parentPathLabels[i]
            if (item.pathLabel !== pathLabels[i])
                item.pathLabel = pathLabels[i]
            if (item.flatIndex !== i)
                item.flatIndex = i
            if (item.visibleIndex !== visibleIndices[i])
                item.visibleIndex = visibleIndices[i]
            if (item.siblingIndex !== siblingIndices[i])
                item.siblingIndex = siblingIndices[i]
            if (item.visibleSiblingIndex !== visibleSiblingIndices[i])
                item.visibleSiblingIndex = visibleSiblingIndices[i]
            if (item.childCount !== childCounts[i])
                item.childCount = childCounts[i]
            if (item.visibleChildCount !== visibleChildCounts[i])
                item.visibleChildCount = visibleChildCounts[i]
            if (item.descendantCount !== descendantCounts[i])
                item.descendantCount = descendantCounts[i]
            if (item.visibleDescendantCount !== visibleDescendantCounts[i])
                item.visibleDescendantCount = visibleDescendantCounts[i]
            if (item.siblingCount !== siblingCount)
                item.siblingCount = siblingCount
            if (item.visibleSiblingCount !== visibleSiblingCount)
                item.visibleSiblingCount = visibleSiblingCount
            if (!arrayTextEquals(item.childItemKeys, childKeys[i]))
                item.childItemKeys = childKeys[i].slice()
            if (!arrayTextEquals(item.childItemLabels, childLabels[i]))
                item.childItemLabels = childLabels[i].slice()
            if (!arrayTextEquals(item.ancestorItemKeys, ancestorKeys[i]))
                item.ancestorItemKeys = ancestorKeys[i].slice()
            if (!arrayTextEquals(item.ancestorLabels, ancestorLabels[i]))
                item.ancestorLabels = ancestorLabels[i].slice()
            if (!arrayTextEquals(item.pathItemKeys, pathKeys[i]))
                item.pathItemKeys = pathKeys[i].slice()
            if (!arrayTextEquals(item.pathItemLabels, pathLabelArrays[i]))
                item.pathItemLabels = pathLabelArrays[i].slice()
        }
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
            if (itemCanBecomeActive(item))
                return item
        }
        return null
    }

    function firstInitiallySelectedItemInList(currentItems) {
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (item && item.selected && itemCanBecomeActive(item))
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
                if (itemCanExpandInList(currentItems, candidate, i) && !candidate.expanded)
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
                expandedByDepth[candidateIndent] = !itemCanExpandInList(currentItems, candidate, i) || !!candidate.expanded
                requiredIndent = candidateIndent
            }
        }
    }

    function refreshVisibleRange(currentItems, startIndex, endIndex) {
        const projection = hierarchyModel.projectInteractionState(buildInteractionDescriptors(currentItems))
        applyProjectedMetadata(currentItems, projection)
        return

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

        syncItemChildPresence(currentItems)

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
            expandedByDepth[indent] = !itemCanExpandInList(currentItems, item, i) || !!item.expanded

            if (rowVisible) {
                rangeVisible.push(i)
                if (itemCanBecomeActive(item))
                    rangeVisibleEnabled.push(i)
            }
        }

        _visibleItemIndices = patchIndexRange(_visibleItemIndices, fromIndex, toIndex, rangeVisible)
        _visibleEnabledItemIndices = patchIndexRange(_visibleEnabledItemIndices, fromIndex, toIndex, rangeVisibleEnabled)
        _visibilityCacheInitialized = true
        syncItemExposedMetadata(currentItems)

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
                if (itemCanExpandInList(currentItems, candidate, i) && !candidate.expanded) {
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
        return hierarchyModel.descendantRangeEnd(buildInteractionDescriptors(currentItems), itemIndex)
    }

    function itemAtPositionInList(currentItems, localX, localY) {
        const visibleItems = collectVisibleItems(false)
        for (let i = 0; i < visibleItems.length; i++) {
            const candidate = visibleItems[i]
            if (!candidate)
                continue
            if (localX < candidate.x || localX > candidate.x + candidate.width)
                continue
            if (localY < candidate.y || localY > candidate.y + candidate.height)
                continue
            const candidateIndex = indexOfItemInList(currentItems, candidate)
            if (candidateIndex >= 0)
                return candidate
        }
        return null
    }

    function insertionIndexForYInList(currentItems, localY) {
        const visibleItems = collectVisibleItems(false)
        if (visibleItems.length === 0)
            return 0

        for (let i = 0; i < visibleItems.length; i++) {
            const candidate = visibleItems[i]
            const candidateIndex = indexOfItemInList(currentItems, candidate)
            if (candidateIndex < 0)
                continue
            const midpoint = candidate.y + candidate.height * 0.5
            if (localY < midpoint)
                return candidateIndex
        }

        const lastVisibleItem = visibleItems[visibleItems.length - 1]
        const lastVisibleIndex = indexOfItemInList(currentItems, lastVisibleItem)
        return lastVisibleIndex < 0 ? currentItems.length : lastVisibleIndex + 1
    }

    function adjustedInsertionIndexForDraggedBlock(rawInsertionIndex, sourceStart, sourceEnd) {
        if (sourceStart < 0 || sourceEnd < sourceStart)
            return -1

        const blockSize = sourceEnd - sourceStart + 1
        if (rawInsertionIndex <= sourceStart)
            return rawInsertionIndex
        if (rawInsertionIndex >= sourceEnd + 1)
            return rawInsertionIndex - blockSize
        return -1
    }

    function dragIndentStep() {
        if (_dragItem) {
            const draggedIndentStep = Number(_dragItem.indentStep)
            if (Number.isFinite(draggedIndentStep) && draggedIndentStep > 0)
                return Math.max(1, Math.round(draggedIndentStep))
        }

        const generatedStep = Number(generatedIndentStep)
        if (Number.isFinite(generatedStep) && generatedStep > 0)
            return Math.max(1, Math.round(generatedStep))
        return 8
    }

    function dragBasePadding() {
        if (_dragItem) {
            const draggedPadding = Number(_dragItem.baseLeftPadding)
            if (Number.isFinite(draggedPadding))
                return draggedPadding
        }
        return 8
    }

    function desiredDepthForPointerX(localX) {
        const indentStep = dragIndentStep()
        const depthFromPosition = Math.floor((localX - dragBasePadding() + indentStep * 0.5) / indentStep)
        return Math.max(0, depthFromPosition)
    }

    function remainingItemsWithoutDraggedBlock(currentItems) {
        const remainingItems = []
        for (let i = 0; i < currentItems.length; i++) {
            if (i >= _dragSourceIndex && i <= _dragSourceEndIndex)
                continue
            remainingItems.push(currentItems[i])
        }
        return remainingItems
    }

    function dragTargetAnchorLabel(item, remainingItems) {
        if (!item)
            return ""

        const itemIndex = remainingItems.indexOf(item)
        return itemDisplayLabel(item, itemIndex >= 0 ? itemIndex : 0)
    }

    function resolvedParentItemInRemaining(remainingItems, insertionIndex, depth) {
        if (depth <= 0)
            return null

        for (let i = insertionIndex - 1; i >= 0; i--) {
            const candidate = remainingItems[i]
            if (!candidate)
                continue

            const candidateDepth = itemIndentLevel(candidate)
            if (candidateDepth === depth - 1)
                return candidate
        }

        return null
    }

    function resolvedEditableDropDescriptor(currentItems, insertionIndex, depth) {
        const remainingItems = remainingItemsWithoutDraggedBlock(currentItems)
        const previousItem = insertionIndex > 0 ? remainingItems[insertionIndex - 1] : null
        const nextItem = insertionIndex < remainingItems.length ? remainingItems[insertionIndex] : null
        const parentItem = resolvedParentItemInRemaining(remainingItems, insertionIndex, depth)
        const previousDepth = previousItem ? itemIndentLevel(previousItem) : -1
        const nextDepth = nextItem ? itemIndentLevel(nextItem) : -1

        let modeName = "root"
        let anchorItem = null

        if (parentItem && previousItem === parentItem && depth === previousDepth + 1) {
            modeName = "child"
            anchorItem = parentItem
        } else if (nextItem && nextDepth === depth) {
            modeName = "before"
            anchorItem = nextItem
        } else if (previousItem && previousDepth === depth) {
            modeName = "after"
            anchorItem = previousItem
        } else if (parentItem) {
            modeName = "child"
            anchorItem = parentItem
        } else if (nextItem && nextDepth === 0) {
            modeName = "before"
            anchorItem = nextItem
        } else if (previousItem && previousDepth === 0) {
            modeName = "after"
            anchorItem = previousItem
        }

        return {
            modeName: modeName,
            parentItem: parentItem,
            parentItemKey: parentItem ? effectiveItemKey(parentItem, remainingItems.indexOf(parentItem)) : "",
            parentLabel: parentItem ? dragTargetAnchorLabel(parentItem, remainingItems) : "",
            parentPathLabel: parentItem && parentItem.pathLabel !== undefined && parentItem.pathLabel !== null
                ? String(parentItem.pathLabel)
                : "",
            anchorItem: anchorItem,
            anchorItemKey: anchorItem ? effectiveItemKey(anchorItem, remainingItems.indexOf(anchorItem)) : "",
            anchorLabel: anchorItem ? dragTargetAnchorLabel(anchorItem, remainingItems) : ""
        }
    }

    function clearEditableDragTarget() {
        _dragTargetIndex = -1
        _dragTargetDepth = -1
        _dragTargetModeName = ""
        _dragTargetParentItem = null
        _dragTargetParentItemKey = ""
        _dragTargetParentLabel = ""
        _dragTargetParentPathLabel = ""
        _dragTargetAnchorItem = null
        _dragTargetAnchorItemKey = ""
        _dragTargetAnchorLabel = ""
    }

    function resolvedEditableDepthForInsertionIndex(currentItems, insertionIndex, desiredDepth) {
        if (insertionIndex < 0)
            return null

        const remainingItems = remainingItemsWithoutDraggedBlock(currentItems)
        const previousItem = insertionIndex > 0 ? remainingItems[insertionIndex - 1] : null
        const nextItem = insertionIndex < remainingItems.length ? remainingItems[insertionIndex] : null
        const minDepth = nextItem ? itemIndentLevel(nextItem) : 0
        const maxDepth = previousItem ? itemIndentLevel(previousItem) + 1 : 0
        const boundedDepth = Math.max(minDepth, Math.min(maxDepth, normalizedDepth(desiredDepth, 0)))
        return {
            depth: boundedDepth,
            minDepth: minDepth,
            maxDepth: maxDepth
        }
    }

    function resolvedDragTargetInList(currentItems, rawInsertionIndex, localX) {
        const resolved = hierarchyModel.resolveDragTarget(buildInteractionDescriptors(currentItems),
                                                          _dragSourceIndex,
                                                          _dragSourceEndIndex,
                                                          rawInsertionIndex,
                                                          localX,
                                                          dragIndentStep(),
                                                          dragBasePadding())
        if (!resolved || resolved.insertionIndex === undefined)
            return null
        return {
            insertionIndex: resolved.insertionIndex,
            depth: resolved.depth,
            minDepth: resolved.minDepth,
            maxDepth: resolved.maxDepth,
            modeName: resolved.modeName,
            parentItem: resolveByKeyInList(currentItems, resolved.parentItemKey),
            parentItemKey: resolved.parentItemKey,
            parentLabel: resolved.parentLabel,
            parentPathLabel: resolved.parentPathLabel,
            anchorItem: resolveByKeyInList(currentItems, resolved.anchorItemKey),
            anchorItemKey: resolved.anchorItemKey,
            anchorLabel: resolved.anchorLabel
        }
    }

    function indicatorYForRawInsertionIndex(currentItems, rawInsertionIndex) {
        if (currentItems.length === 0)
            return 0
        if (rawInsertionIndex <= 0)
            return 0
        if (rawInsertionIndex >= currentItems.length) {
            const lastItem = currentItems[currentItems.length - 1]
            return lastItem ? lastItem.y + lastItem.height : 0
        }

        const targetItem = currentItems[rawInsertionIndex]
        return targetItem ? targetItem.y : 0
    }

    function editableDescriptorFromItem(item, index) {
        if (!item || !item.nodeData || typeof item.nodeData !== "object" || Array.isArray(item.nodeData))
            return null

        return {
            nodeData: item.nodeData,
            itemId: effectiveItemId(item, index),
            itemKey: effectiveItemKey(item, index),
            indentLevel: itemIndentLevel(item),
            label: item.label === undefined || item.label === null ? "" : String(item.label),
            iconName: item.iconName === undefined || item.iconName === null ? "" : String(item.iconName),
            iconSource: item.iconSource === undefined || item.iconSource === null ? "" : String(item.iconSource),
            iconGlyph: item.iconGlyph === undefined || item.iconGlyph === null ? "" : String(item.iconGlyph),
            enabled: !!item.enabled,
            activatable: item.activatable === undefined ? true : !!item.activatable,
            draggable: item.dragAllowed === undefined ? true : !!item.dragAllowed,
            expanded: !!item.expanded,
            showChevron: !!item.showChevron
        }
    }

    function buildEditableDescriptors(currentItems) {
        const descriptors = []
        for (let i = 0; i < currentItems.length; i++) {
            const descriptor = editableDescriptorFromItem(currentItems[i], i)
            if (!descriptor)
                return null
            descriptors.push(descriptor)
        }
        return descriptors
    }

    function interactionDescriptorFromItem(item, index) {
        if (!item)
            return null
        return {
            itemId: effectiveItemId(item, index),
            itemKey: effectiveItemKey(item, index),
            label: itemDisplayLabel(item, index),
            pathLabel: item.pathLabel === undefined || item.pathLabel === null ? "" : String(item.pathLabel),
            indentLevel: itemIndentLevel(item),
            enabled: !!item.enabled,
            activatable: item.activatable === undefined ? true : !!item.activatable,
            draggable: item.dragAllowed === undefined ? true : !!item.dragAllowed,
            expanded: !!item.expanded,
            selected: !!item.selected,
            showChevron: !!item.showChevron,
            x: item.x,
            y: item.y,
            width: item.width,
            height: item.height,
            nodeData: item.nodeData
        }
    }

    function buildInteractionDescriptors(currentItems) {
        const descriptors = []
        for (let i = 0; i < currentItems.length; i++) {
            const descriptor = interactionDescriptorFromItem(currentItems[i], i)
            if (descriptor)
                descriptors.push(descriptor)
        }
        return descriptors
    }

    function applyProjectedMetadata(currentItems, projection) {
        if (!projection)
            return
        _visibilityFlags = modelList(projection.visibilityFlags)
        _visibleItemIndices = modelList(projection.visibleItemIndices)
        _visibleEnabledItemIndices = modelList(projection.visibleEnabledItemIndices)
        _idIndexMap = projection.idIndexMap || ({})
        _keyIndexMap = projection.keyIndexMap || ({})
        _visibilityCacheInitialized = true
        _itemCountInternal = projection.itemCount || currentItems.length
        _visibleItemCountInternal = projection.visibleItemCount || _visibleItemIndices.length

        const metadata = modelList(projection.metadata)
        for (let i = 0; i < currentItems.length && i < metadata.length; i++) {
            const item = currentItems[i]
            const row = metadata[i]
            if (!item || !row)
                continue
            if (item.hierarchyList !== control)
                item.hierarchyList = control
            if (item.hasChildItems !== undefined && item.hasChildItems !== row.hasChildren)
                item.hasChildItems = row.hasChildren
            if (item._rowVisibleInternal !== row.visible)
                item._rowVisibleInternal = row.visible
            if (item.parentItemKey !== row.parentItemKey)
                item.parentItemKey = row.parentItemKey
            if (item.parentLabel !== row.parentLabel)
                item.parentLabel = row.parentLabel
            if (item.parentPathLabel !== row.parentPathLabel)
                item.parentPathLabel = row.parentPathLabel
            if (item.pathLabel !== row.pathLabel)
                item.pathLabel = row.pathLabel
            if (item.flatIndex !== row.flatIndex)
                item.flatIndex = row.flatIndex
            if (item.visibleIndex !== row.visibleIndex)
                item.visibleIndex = row.visibleIndex
            if (item.siblingIndex !== row.siblingIndex)
                item.siblingIndex = row.siblingIndex
            if (item.visibleSiblingIndex !== row.visibleSiblingIndex)
                item.visibleSiblingIndex = row.visibleSiblingIndex
            if (item.childCount !== row.childCount)
                item.childCount = row.childCount
            if (item.visibleChildCount !== row.visibleChildCount)
                item.visibleChildCount = row.visibleChildCount
            if (item.descendantCount !== row.descendantCount)
                item.descendantCount = row.descendantCount
            if (item.visibleDescendantCount !== row.visibleDescendantCount)
                item.visibleDescendantCount = row.visibleDescendantCount
            if (item.siblingCount !== row.siblingCount)
                item.siblingCount = row.siblingCount
            if (item.visibleSiblingCount !== row.visibleSiblingCount)
                item.visibleSiblingCount = row.visibleSiblingCount
            const childKeys = modelList(row.childItemKeys)
            const childLabels = modelList(row.childItemLabels)
            const ancestorKeys = modelList(row.ancestorItemKeys)
            const ancestorLabels = modelList(row.ancestorLabels)
            const pathKeys = modelList(row.pathItemKeys)
            const pathLabels = modelList(row.pathItemLabels)
            if (!arrayTextEquals(item.childItemKeys, childKeys))
                item.childItemKeys = childKeys
            if (!arrayTextEquals(item.childItemLabels, childLabels))
                item.childItemLabels = childLabels
            if (!arrayTextEquals(item.ancestorItemKeys, ancestorKeys))
                item.ancestorItemKeys = ancestorKeys
            if (!arrayTextEquals(item.ancestorLabels, ancestorLabels))
                item.ancestorLabels = ancestorLabels
            if (!arrayTextEquals(item.pathItemKeys, pathKeys))
                item.pathItemKeys = pathKeys
            if (!arrayTextEquals(item.pathItemLabels, pathLabels))
                item.pathItemLabels = pathLabels
        }
    }

    function assignNodeDescriptorState(node, descriptor, parentDescriptor) {
        const keyRoleName = normalizedRoleName(itemKeyRole, "key")
        const idRoleName = normalizedRoleName(itemIdRole, "itemId")
        const labelRoleName = normalizedRoleName(labelRole, "label")
        const iconNameRoleName = normalizedRoleName(iconNameRole, "iconName")
        const iconSourceRoleName = normalizedRoleName(iconSourceRole, "iconSource")
        const iconGlyphRoleName = normalizedRoleName(iconGlyphRole, "iconGlyph")
        const enabledRoleName = normalizedRoleName(enabledRole, "enabled")
        const activatableRoleName = normalizedRoleName(activatableRole, "activatable")
        const draggableRoleName = normalizedRoleName(draggableRole, "draggable")
        const expandedRoleName = normalizedRoleName(expandedRole, "expanded")
        const showChevronRoleName = normalizedRoleName(showChevronRole, "showChevron")
        const depthRoleName = normalizedRoleName(depthRole, "depth")
        const parentItemKey = parentDescriptor && parentDescriptor.itemKey !== undefined && parentDescriptor.itemKey !== null
            ? String(parentDescriptor.itemKey)
            : ""

        if (keyRoleName.length > 0)
            node[keyRoleName] = descriptor.itemKey
        if (descriptor.itemId >= 0 && idRoleName.length > 0)
            node[idRoleName] = descriptor.itemId
        if (labelRoleName.length > 0)
            node[labelRoleName] = descriptor.label
        if (iconNameRoleName.length > 0 && (descriptor.iconName.length > 0 || node[iconNameRoleName] !== undefined))
            node[iconNameRoleName] = descriptor.iconName
        if (iconSourceRoleName.length > 0 && (descriptor.iconSource.length > 0 || node[iconSourceRoleName] !== undefined))
            node[iconSourceRoleName] = descriptor.iconSource
        if (iconGlyphRoleName.length > 0 && (descriptor.iconGlyph.length > 0 || node[iconGlyphRoleName] !== undefined))
            node[iconGlyphRoleName] = descriptor.iconGlyph
        if (enabledRoleName.length > 0)
            node[enabledRoleName] = descriptor.enabled
        if (activatableRoleName.length > 0)
            node[activatableRoleName] = descriptor.activatable
        if (node.activatable !== undefined || activatableRoleName === "activatable")
            node.activatable = descriptor.activatable
        if (node.selectable !== undefined || activatableRoleName === "selectable")
            node.selectable = descriptor.activatable
        if (draggableRoleName.length > 0)
            node[draggableRoleName] = descriptor.draggable
        if (node.dragAllowed !== undefined || draggableRoleName === "dragAllowed")
            node.dragAllowed = descriptor.draggable
        if (node.draggable !== undefined || draggableRoleName === "draggable")
            node.draggable = descriptor.draggable
        if (expandedRoleName.length > 0)
            node[expandedRoleName] = descriptor.expanded
        if (showChevronRoleName.length > 0)
            node[showChevronRoleName] = descriptor.showChevron

        if (node.indentLevel !== undefined || depthRoleName === "indentLevel")
            node.indentLevel = descriptor.indentLevel
        if (depthRoleName.length > 0)
            node[depthRoleName] = descriptor.indentLevel

        node.parentKey = parentItemKey
        node.parentItemKey = parentItemKey
    }

    function applyEditableTreeOrder(flatDescriptors) {
        if (!Array.isArray(model) || !Array.isArray(flatDescriptors))
            return false

        for (let i = 0; i < flatDescriptors.length; i++) {
            const descriptor = flatDescriptors[i]
            if (!descriptor || !descriptor.nodeData)
                return false
        }

        for (let i = 0; i < flatDescriptors.length; i++) {
            const descriptor = flatDescriptors[i]
            let parentDescriptor = null
            for (let j = i - 1; j >= 0; j--) {
                const candidate = flatDescriptors[j]
                if (candidate.indentLevel < descriptor.indentLevel) {
                    parentDescriptor = candidate
                    break
                }
            }
            assignNodeDescriptorState(descriptor.nodeData, descriptor, parentDescriptor)
        }

        while (model.length > 0)
            model.pop()
        for (let i = 0; i < flatDescriptors.length; i++)
            model.push(flatDescriptors[i].nodeData)

        return true
    }

    function qmlListModelSupportsEditing() {
        return model !== undefined
            && model !== null
            && !Array.isArray(model)
            && typeof model === "object"
            && typeof model.move === "function"
            && typeof model.setProperty === "function"
    }

    function parentKeyForEditableDescriptorAt(flatDescriptors, row) {
        if (!Array.isArray(flatDescriptors) || row < 0 || row >= flatDescriptors.length)
            return ""
        const descriptor = flatDescriptors[row]
        const depth = descriptor ? normalizedDepth(descriptor.indentLevel, 0) : 0
        if (depth <= 0)
            return ""
        for (let i = row - 1; i >= 0; i--) {
            const candidate = flatDescriptors[i]
            if (candidate && normalizedDepth(candidate.indentLevel, 0) < depth)
                return candidate.itemKey === undefined || candidate.itemKey === null ? "" : String(candidate.itemKey)
        }
        return ""
    }

    function applyEditableListModelMove(moveResult) {
        if (!qmlListModelSupportsEditing() || !moveResult || !moveResult.accepted)
            return false

        const reorderedDescriptors = modelList(moveResult.reorderedDescriptors)
        const toIndex = normalizeFromIndex(moveResult.toIndex)
        const blockSize = Math.max(0, _dragSourceEndIndex - _dragSourceIndex + 1)
        if (!Array.isArray(reorderedDescriptors) || blockSize <= 0
                || toIndex < 0 || toIndex + blockSize > reorderedDescriptors.length) {
            return false
        }

        if (toIndex !== _dragSourceIndex)
            model.move(_dragSourceIndex, toIndex, blockSize)

        const depthRoleName = normalizedRoleName(depthRole, "depth")
        for (let row = toIndex; row < toIndex + blockSize; row++) {
            const descriptor = reorderedDescriptors[row]
            if (!descriptor)
                return false
            const depth = normalizedDepth(descriptor.indentLevel, 0)
            const node = descriptor.nodeData
            if (node && typeof node === "object" && node.indentLevel !== undefined)
                model.setProperty(row, "indentLevel", depth)
            if (depthRoleName.length > 0)
                model.setProperty(row, depthRoleName, depth)

            const parentItemKey = parentKeyForEditableDescriptorAt(reorderedDescriptors, row)
            if (node && typeof node === "object" && node.parentKey !== undefined)
                model.setProperty(row, "parentKey", parentItemKey)
            if (node && typeof node === "object" && node.parentItemKey !== undefined)
                model.setProperty(row, "parentItemKey", parentItemKey)
        }

        return true
    }

    function _rawInsertionIndexForDropMode(currentItems, targetItem, dropModeName) {
        const normalizedMode = dropModeName === undefined || dropModeName === null
            ? ""
            : String(dropModeName).trim().toLowerCase()
        if (normalizedMode === "root")
            return currentItems.length

        const targetIndex = indexOfItemInList(currentItems, targetItem)
        if (targetIndex < 0)
            return -1

        if (normalizedMode === "before")
            return targetIndex
        if (normalizedMode === "after")
            return descendantRangeEndInList(currentItems, targetIndex) + 1
        if (normalizedMode === "child")
            return targetIndex + 1

        return -1
    }

    function _targetDepthForDropMode(targetItem, dropModeName) {
        const normalizedMode = dropModeName === undefined || dropModeName === null
            ? ""
            : String(dropModeName).trim().toLowerCase()
        if (normalizedMode === "root")
            return 0
        if (!targetItem)
            return -1

        const baseDepth = itemIndentLevel(targetItem)
        if (normalizedMode === "child")
            return baseDepth + 1
        if (normalizedMode === "before" || normalizedMode === "after")
            return baseDepth
        return -1
    }

    function _emitDragUpdated(item, resolvedTarget) {
        if (!item || !resolvedTarget || item.dragUpdated === undefined)
            return

        item.dragUpdated(resolvedTarget.insertionIndex,
                         resolvedTarget.depth,
                         resolvedTarget.modeName,
                         resolvedTarget.parentItemKey,
                         resolvedTarget.anchorItemKey)
    }

    function _emitDragStarted(item) {
        if (!item || item.dragStarted === undefined)
            return

        item.dragStarted(_dragSourceIndex,
                         _dragSourceEndIndex,
                         itemIndentLevel(item))
    }

    function _emitDragEnded(item, committed, fromIndex, toIndex, depth, modeName, parentItemKey, anchorItemKey) {
        if (!item || item.dragEnded === undefined)
            return

        item.dragEnded(!!committed,
                       fromIndex,
                       toIndex,
                       depth,
                       modeName,
                       parentItemKey,
                       anchorItemKey)
    }

    function _applyEditableMoveByRawInsertion(item, rawInsertionIndex, targetDepth) {
        if (!editableEnabled || !usingTreeModel || !item)
            return false

        const currentItems = collectItems()
        const sourceIndex = indexOfItemInList(currentItems, item)
        if (sourceIndex < 0)
            return false

        const sourceEndIndex = descendantRangeEndInList(currentItems, sourceIndex)
        const adjustedIndex = adjustedInsertionIndexForDraggedBlock(rawInsertionIndex,
                                                                   sourceIndex,
                                                                   sourceEndIndex)
        if (adjustedIndex < 0)
            return false

        _dragSourceIndex = sourceIndex
        _dragSourceEndIndex = sourceEndIndex
        return _applyEditableMove(item, adjustedIndex, targetDepth)
    }

    function _applyEditableMoveByDropMode(item, targetItem, dropModeName) {
        const currentItems = collectItems()
        const rawInsertionIndex = _rawInsertionIndexForDropMode(currentItems, targetItem, dropModeName)
        if (rawInsertionIndex < 0)
            return false

        const targetDepth = _targetDepthForDropMode(targetItem, dropModeName)
        if (targetDepth < 0)
            return false

        return _applyEditableMoveByRawInsertion(item, rawInsertionIndex, targetDepth)
    }

    function _applyEditableMove(item, targetIndex, targetDepth) {
        if (!editableEnabled || !usingTreeModel || !item)
            return false

        const currentItems = collectItems()
        const sourceIndex = indexOfItemInList(currentItems, item)
        if (sourceIndex < 0)
            return false

        _dragSourceIndex = sourceIndex
        _dragSourceEndIndex = descendantRangeEndInList(currentItems, sourceIndex)

        const editableDescriptors = buildEditableDescriptors(currentItems)
        if (!editableDescriptors)
            return false

        const normalizedTargetIndex = normalizeFromIndex(targetIndex)
        const normalizedTargetDepth = normalizedDepth(targetDepth, 0)
        const moveResult = Array.isArray(model) || qmlListModelSupportsEditing()
            ? hierarchyModel.moveDescriptors(editableDescriptors,
                                             _dragSourceIndex,
                                             _dragSourceEndIndex,
                                             normalizedTargetIndex,
                                             normalizedTargetDepth)
            : hierarchyModel.moveSourceRows(_dragSourceIndex,
                                            _dragSourceEndIndex,
                                            normalizedTargetIndex,
                                            normalizedTargetDepth)
        if (!moveResult || !moveResult.accepted)
            return false

        if (Array.isArray(model)) {
            const reorderedDescriptors = modelList(moveResult.reorderedDescriptors)
            if (!applyEditableTreeOrder(reorderedDescriptors))
                return false
        } else if (qmlListModelSupportsEditing()) {
            if (!applyEditableListModelMove(moveResult))
                return false
        }

        const movedDescriptor = moveResult.movedDescriptor
        const dropDescriptor = moveResult.drop || ({})
        markItemsDirty(true)
        scheduleRebuildTreeItems()
        scheduleRefreshState()
        scheduleNormalizeActiveItem()
        itemMoved(item,
                  movedDescriptor.itemId,
                  movedDescriptor.itemKey,
                  _dragSourceIndex,
                  moveResult.toIndex,
                  movedDescriptor.indentLevel)
        _emitDragEnded(item,
                       true,
                       _dragSourceIndex,
                       moveResult.toIndex,
                       movedDescriptor.indentLevel,
                       dropDescriptor.modeName,
                       dropDescriptor.parentItemKey,
                       dropDescriptor.anchorItemKey)
        return true
    }

    function resetEditableDragState() {
        if (_dragItem && _dragItem.dragPreviewActive !== undefined)
            _dragItem.dragPreviewActive = false

        _dragActiveInternal = false
        _dragItem = null
        _dragSourceIndex = -1
        _dragSourceEndIndex = -1
        clearEditableDragTarget()
        _dragRawInsertionIndex = -1
        _dragPointerX = 0
        _dragPointerY = 0
        _dragIndicatorY = 0
        _dragIndicatorX = 0
    }

    function _beginEditableDragForItem(draggedItem, localX, localY) {
        if (!editableEnabled || !usingTreeModel)
            return false

        const currentItems = collectItems()
        if (buildEditableDescriptors(currentItems) === null)
            return false

        if (!draggedItem || !draggedItem.enabled)
            return false

        const sourceIndex = indexOfItemInList(currentItems, draggedItem)
        if (sourceIndex < 0)
            return false

        resetEditableDragState()

        _dragActiveInternal = true
        _dragItem = draggedItem
        _dragSourceIndex = sourceIndex
        _dragSourceEndIndex = descendantRangeEndInList(currentItems, sourceIndex)
        _dragPointerX = localX
        _dragPointerY = localY
        if (_dragItem.dragPreviewActive !== undefined)
            _dragItem.dragPreviewActive = true
        _emitDragStarted(_dragItem)
        _updateEditableDrag(localX, localY)
        return true
    }

    function _beginEditableDragAtPosition(localX, localY) {
        const currentItems = collectItems()
        const draggedItem = itemAtPositionInList(currentItems, localX, localY)
        return _beginEditableDragForItem(draggedItem, localX, localY)
    }

    function _updateEditableDrag(localX, localY) {
        if (!_dragActiveInternal || !_dragItem)
            return false

        _dragPointerX = localX
        _dragPointerY = localY

        const currentItems = collectItems()
        const rawInsertionIndex = insertionIndexForYInList(currentItems, localY)
        const resolvedTarget = resolvedDragTargetInList(currentItems, rawInsertionIndex, localX)
        _dragRawInsertionIndex = rawInsertionIndex

        if (!resolvedTarget) {
            clearEditableDragTarget()
            return false
        }

        _dragTargetIndex = resolvedTarget.insertionIndex
        _dragTargetDepth = resolvedTarget.depth
        _dragTargetModeName = resolvedTarget.modeName
        _dragTargetParentItem = resolvedTarget.parentItem
        _dragTargetParentItemKey = resolvedTarget.parentItemKey
        _dragTargetParentLabel = resolvedTarget.parentLabel
        _dragTargetParentPathLabel = resolvedTarget.parentPathLabel
        _dragTargetAnchorItem = resolvedTarget.anchorItem
        _dragTargetAnchorItemKey = resolvedTarget.anchorItemKey
        _dragTargetAnchorLabel = resolvedTarget.anchorLabel
        _dragIndicatorY = indicatorYForRawInsertionIndex(currentItems, rawInsertionIndex)
        _dragIndicatorX = dragBasePadding() + resolvedTarget.depth * dragIndentStep()
        _emitDragUpdated(_dragItem, resolvedTarget)
        return true
    }

    function _applyDraggedItemMove() {
        if (!_dragActiveInternal
                || !_dragItem
                || _dragTargetIndex < 0
                || _dragTargetDepth < 0) {
            return false
        }
        return _applyEditableMove(_dragItem, _dragTargetIndex, _dragTargetDepth)
    }

    function _endEditableDrag(commitMove) {
        const shouldCommit = commitMove === undefined ? true : !!commitMove
        if (!_dragActiveInternal) {
            resetEditableDragState()
            return false
        }

        const draggedItem = _dragItem
        const fromIndex = _dragSourceIndex
        const toIndex = _dragTargetIndex
        const targetDepth = _dragTargetDepth
        const modeName = _dragTargetModeName
        const parentItemKey = _dragTargetParentItemKey
        const anchorItemKey = _dragTargetAnchorItemKey
        const committed = shouldCommit ? _applyDraggedItemMove() : false
        if (!committed)
            _emitDragEnded(draggedItem,
                           false,
                           fromIndex,
                           toIndex,
                           targetDepth,
                           modeName,
                           parentItemKey,
                           anchorItemKey)
        resetEditableDragState()
        return committed
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

    function applyActiveState(item, index, emitSignal, forceSignal) {
        const nextItem = item || null
        const nextIndex = nextItem ? index : -1
        const nextId = nextItem ? effectiveItemId(nextItem, nextIndex) : -1
        const nextKey = nextItem ? effectiveItemKey(nextItem, nextIndex) : ""

        const changed = activeItem !== nextItem || activeItemId !== nextId || activeItemKey !== nextKey
        const shouldEmit = !!emitSignal && (changed || !!forceSignal)
        if (!changed && !shouldEmit)
            return false

        if (changed) {
            _applyingActiveState = true
            activeItem = nextItem
            activeItemId = nextId
            activeItemKey = nextKey
            _applyingActiveState = false
        }

        if (nextItem && (changed || !!forceSignal))
            ensureVisibleRequested(nextItem.y, nextItem.height)

        if (shouldEmit)
            activeChanged(nextItem, nextId, nextIndex)

        return changed || !!forceSignal
    }

    function requestActivate(item, forceSignalIfAlreadyActive) {
        if (!item || !itemCanBecomeActive(item) || !isManagedItem(item))
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

        const activated = applyActiveState(item, index, true, !!forceSignalIfAlreadyActive)
        if (activated && keyboardNavigationEnabled && !control.activeFocus)
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
        const normalizedKey = itemKey === undefined || itemKey === null ? "" : String(itemKey).trim()
        if (normalizedKey.length === 0)
            return false

        const item = resolveByKey(normalizedKey)
        if (!item) {
            if (usingTreeModel && (_isBuildingGeneratedItems || _rebuildScheduled || _itemsDirty)) {
                Qt.callLater(function() {
                    control.activateByKey(normalizedKey)
                })
            }
            return false
        }

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

        if (!targetItem || indexOfItemInList(currentItems, targetItem) === -1 || !itemCanBecomeActive(targetItem))
            targetItem = firstInitiallySelectedItemInList(currentItems)
        if (!targetItem || !itemCanBecomeActive(targetItem))
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

    function buildModelDescriptors(nodes, depth, parentKey, parentPath, sink) {
        const descriptors = hierarchyModel.descriptors
        for (let i = 0; i < descriptors.length; i++)
            sink.push(descriptors[i])
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
            normalizeActiveItem()
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
                                                                 count: descriptor.count,
                                                                 showChevron: descriptor.showChevron,
                                                                 hasChildItems: descriptor.hasChildren,
                                                                 expanded: descriptor.expanded,
                                                                 selected: descriptor.selected,
                                                                 enabled: descriptor.enabled,
                                                                 activatable: descriptor.activatable,
                                                                 dragAllowed: descriptor.draggable,
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
        normalizeActiveItem()
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

        const descriptors = []
        buildModelDescriptors(model, 0, "", "", descriptors)
        _rebuildDescriptors = descriptors
        _rebuildDescriptorIndex = 0
        scheduleRebuildChunk(revision)
    }

    function expandAll() {
        const currentItems = collectItems()
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (item && itemCanExpandInList(currentItems, item, i) && !item.expanded)
                item.expanded = true
        }
        scheduleRefreshState()
    }

    function collapseAll(keepRootExpanded) {
        const keepRoot = keepRootExpanded === undefined ? true : !!keepRootExpanded
        const currentItems = collectItems()
        for (let i = 0; i < currentItems.length; i++) {
            const item = currentItems[i]
            if (!item || !itemCanExpandInList(currentItems, item, i))
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

        const currentItems = collectItems()
        const activeIndex = indexOfItemInList(currentItems, activeItem)
        if (activeIndex < 0)
            return false

        if (itemCanExpandInList(currentItems, activeItem, activeIndex) && activeItem.expanded) {
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

        const currentItems = collectItems()
        const activeIndex = indexOfItemInList(currentItems, activeItem)
        if (activeIndex < 0)
            return false

        if (itemCanExpandInList(currentItems, activeItem, activeIndex) && !activeItem.expanded) {
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
        if (_dragActiveInternal)
            _endEditableDrag(false)
        markItemsDirty(true)
        scheduleRebuildTreeItems()
        scheduleRefreshState()
    }
    onModelChanged: {
        invalidateModel()
    }
    onModelColumnChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onItemIdRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onItemKeyRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onLabelRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onIconNameRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onIconSourceRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onIconGlyphRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onEnabledRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onExpandedRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onSelectedRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onActivatableRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onShowChevronRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onDepthRoleChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onGeneratedIndentStepChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onGeneratedRowHeightChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onGeneratedItemWidthChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onGeneratedIconSizeChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onGeneratedChevronSizeChanged: { markItemsDirty(true); scheduleRebuildTreeItems() }
    onEditableChanged: {
        if (!editable && _dragActiveInternal)
            _endEditableDrag(false)
    }

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

    Rectangle {
        visible: control._dragActiveInternal && control._dragTargetIndex >= 0 && control._dragTargetDepth >= 0
        x: Math.max(0, control._dragIndicatorX)
        y: Math.max(0, control._dragIndicatorY - Theme.scaleMetric(1))
        width: Math.max(Theme.scaleMetric(24), control.width - x - Theme.gap8)
        height: Theme.scaleMetric(2)
        radius: Theme.scaleMetric(1)
        color: Theme.primary
        z: 50
    }

    Rectangle {
        visible: control._dragActiveInternal && control._dragTargetIndex >= 0 && control._dragTargetDepth >= 0
        width: Theme.scaleMetric(8)
        height: Theme.scaleMetric(8)
        radius: Theme.scaleMetric(4)
        x: Math.max(0, control._dragIndicatorX - width * 0.5)
        y: Math.max(0, control._dragIndicatorY - height * 0.5)
        color: Theme.primary
        z: 51
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

    Connections {
        target: hierarchyModel

        function onDescriptorsChanged() {
            control.invalidateModel()
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
//         { key: "world", depth: 0, label: "World", expanded: true },
//         { key: "camera", depth: 1, label: "Camera" }
//     ]
// }
