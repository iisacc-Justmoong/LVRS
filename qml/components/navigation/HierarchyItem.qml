import QtQuick
import QtQuick.Layouts
import LVRS 1.0

AbstractButton {
    id: control

    readonly property bool __isHierarchyItem: true

    readonly property int directionRight: 0
    readonly property int directionLeft: 1
    readonly property int directionUp: 2
    readonly property int directionDown: 3

    readonly property int uxStateIdle: 0
    readonly property int uxStateHover: 1
    readonly property int uxStateActive: 2
    readonly property int uxStateInactive: 3
    readonly property int uxStatePressed: 4
    readonly property int uxStateDrag: 5

    readonly property int stateIdle: 0
    readonly property int stateHover: 1
    readonly property int stateActive: 2

    readonly property int dropModeNone: -1
    readonly property int dropModeBefore: 0
    readonly property int dropModeAfter: 1
    readonly property int dropModeChild: 2
    readonly property int dropModeRoot: 3

    property int itemId: -1
    property string itemKey: ""
    property string parentItemKey: ""
    property string parentLabel: ""
    property string pathLabel: ""
    property string parentPathLabel: ""
    property var hierarchyList: null
    property var nodeData: null
    property bool generatedByTreeModel: false

    property int childCount: 0
    property int visibleChildCount: 0
    property int descendantCount: 0
    property int visibleDescendantCount: 0
    property var childItemKeys: []
    property var childItemLabels: []
    property var ancestorItemKeys: []
    property var ancestorLabels: []
    property var pathItemKeys: []
    property var pathItemLabels: []
    property int flatIndex: -1
    property int visibleIndex: -1
    property int siblingIndex: -1
    property int visibleSiblingIndex: -1
    property int siblingCount: 0
    property int visibleSiblingCount: 0

    text: "Label"
    property alias label: control.text
    property string iconName: ""
    property url iconSource: ""
    property string iconGlyph: ""
    property bool showChevron: true
    property bool hasChildItems: true
    property bool expanded: false
    property var selectionDirection: "auto"
    property bool selected: false

    QtObject {
        id: activationCapabilities
        property bool value: true
    }

    property alias activatable: activationCapabilities.value
    property alias selectable: activationCapabilities.value

    property bool dragPreviewActive: false
    property real dragPreviewOpacity: 0.45

    property int indentLevel: 0
    property int indentStep: 8
    property int rowHeight: 20
    property int itemWidth: 200
    property int iconSize: 16
    property int chevronSize: 16
    property int baseLeftPadding: Theme.gap8
    property int rowRightPadding: Theme.gap8
    property int leadingSpacing: Theme.gap2

    property color iconPlaceholderColor: Theme.darkGrey10
    property color textColorNormal: Theme.bodyColor
    property color textColorDisabled: Theme.disabledColor
    property color rowBackgroundColorIdle: "transparent"
    property color rowBackgroundColorHover: Theme.surfaceGhost
    property color rowBackgroundColorPressed: Theme.surfaceAlt
    property color rowBackgroundColorActive: Theme.accentBlueMuted
    property color rowBackgroundColorInactive: Theme.panelBackground12
    property color rowBackgroundColorDrag: Theme.accentBlueMuted
    property bool _rowVisibleInternal: true
    readonly property bool rowVisible: _rowVisibleInternal

    signal dragStarted(int sourceIndex, int sourceEndIndex, int sourceDepth)
    signal dragUpdated(int targetIndex, int targetDepth, string modeName, string parentItemKey, string anchorItemKey)
    signal dragEnded(bool committed, int fromIndex, int toIndex, int targetDepth, string modeName, string parentItemKey, string anchorItemKey)

    readonly property int treeDepth: Math.max(0, indentLevel)
    readonly property int resolvedItemId: itemId >= 0 ? itemId : flatIndex
    readonly property string resolvedItemKey: {
        const explicitKey = normalizedText(itemKey).trim()
        if (explicitKey.length > 0)
            return explicitKey
        if (resolvedItemId >= 0)
            return String(resolvedItemId)
        return ""
    }
    readonly property string resolvedLabel: {
        const explicitLabel = normalizedText(control.text)
        if (explicitLabel.length > 0)
            return explicitLabel
        return control.resolvedItemKey
    }
    readonly property string resolvedPathLabel: {
        const explicitPath = normalizedText(pathLabel)
        if (explicitPath.length > 0)
            return explicitPath
        return control.resolvedLabel
    }
    readonly property bool hasParentItem: normalizedText(parentItemKey).trim().length > 0
        || normalizedText(parentLabel).trim().length > 0
        || treeDepth > 0
    readonly property int computedLeftPadding: baseLeftPadding + treeDepth * indentStep
    readonly property int effectiveChildCount: Math.max(childCount,
                                                        normalizedStringArray(childItemKeys).length,
                                                        normalizedStringArray(childItemLabels).length)
    readonly property int hiddenChildCount: Math.max(0, childCount - visibleChildCount)
    readonly property int hiddenDescendantCount: Math.max(0, descendantCount - visibleDescendantCount)
    readonly property bool effectiveHasChildItems: hasChildItems || effectiveChildCount > 0
    readonly property bool hasVisibleChildItems: visibleChildCount > 0
    readonly property bool hasHiddenChildItems: hiddenChildCount > 0
    readonly property bool hasVisibleDescendants: visibleDescendantCount > 0
    readonly property bool hasHiddenDescendants: hiddenDescendantCount > 0
    readonly property bool effectiveShowChevron: showChevron && effectiveHasChildItems
    readonly property bool chevronExpandable: effectiveShowChevron
    readonly property bool collapsed: !expanded
    readonly property bool canToggleExpanded: chevronExpandable && enabled
    readonly property bool canExpand: canToggleExpanded && collapsed
    readonly property bool canCollapse: canToggleExpanded && expanded
    readonly property bool canBecomeActive: activatable && enabled
    readonly property bool active: resolvedSelected
    readonly property bool inactive: !enabled || !activatable
    readonly property bool draggable: hierarchyList
        ? !!(hierarchyList.editableEnabled !== undefined ? hierarchyList.editableEnabled : hierarchyList.editable)
        : false
    readonly property bool dragEnabled: draggable && enabled && generatedByTreeModel
    readonly property bool dragging: dragPreviewActive
    readonly property bool dragActive: hierarchyList
        ? !!hierarchyList._dragActiveInternal && hierarchyList._dragItem === control
        : dragging
    readonly property bool dragTargetValid: hierarchyList
        ? !!hierarchyList._dragActiveInternal && hierarchyList._dragTargetIndex >= 0 && hierarchyList._dragTargetDepth >= 0
        : false
    readonly property bool dragSourceItem: hierarchyList ? hierarchyList._dragItem === control : false
    readonly property bool dragAnchorItem: hierarchyList ? hierarchyList._dragTargetAnchorItem === control : false
    readonly property bool dragParentTargetItem: hierarchyList ? hierarchyList._dragTargetParentItem === control : false
    readonly property int dragSourceIndex: hierarchyList && hierarchyList._dragItem === control
        ? hierarchyList._dragSourceIndex
        : flatIndex
    readonly property int dragSourceEndIndex: hierarchyList && hierarchyList._dragItem === control
        ? hierarchyList._dragSourceEndIndex
        : flatIndex
    readonly property int dragTargetIndex: hierarchyList ? hierarchyList._dragTargetIndex : -1
    readonly property int dragTargetDepth: hierarchyList ? hierarchyList._dragTargetDepth : -1
    readonly property string dragTargetModeName: hierarchyList
        ? (hierarchyList._dragTargetModeName === undefined || hierarchyList._dragTargetModeName === null
               ? ""
               : String(hierarchyList._dragTargetModeName))
        : ""
    readonly property int dragTargetMode: {
        const modeName = dragTargetModeName.trim().toLowerCase()
        if (modeName === "before")
            return dropModeBefore
        if (modeName === "after")
            return dropModeAfter
        if (modeName === "child")
            return dropModeChild
        if (modeName === "root")
            return dropModeRoot
        return dropModeNone
    }
    readonly property string dragTargetParentItemKey: hierarchyList
        ? (hierarchyList._dragTargetParentItemKey === undefined || hierarchyList._dragTargetParentItemKey === null
               ? ""
               : String(hierarchyList._dragTargetParentItemKey))
        : ""
    readonly property string dragTargetParentLabel: hierarchyList
        ? (hierarchyList._dragTargetParentLabel === undefined || hierarchyList._dragTargetParentLabel === null
               ? ""
               : String(hierarchyList._dragTargetParentLabel))
        : ""
    readonly property string dragTargetParentPathLabel: hierarchyList
        ? (hierarchyList._dragTargetParentPathLabel === undefined || hierarchyList._dragTargetParentPathLabel === null
               ? ""
               : String(hierarchyList._dragTargetParentPathLabel))
        : ""
    readonly property string dragTargetAnchorItemKey: hierarchyList
        ? (hierarchyList._dragTargetAnchorItemKey === undefined || hierarchyList._dragTargetAnchorItemKey === null
               ? ""
               : String(hierarchyList._dragTargetAnchorItemKey))
        : ""
    readonly property string dragTargetAnchorLabel: hierarchyList
        ? (hierarchyList._dragTargetAnchorLabel === undefined || hierarchyList._dragTargetAnchorLabel === null
               ? ""
               : String(hierarchyList._dragTargetAnchorLabel))
        : ""
    readonly property bool dropBefore: dragTargetMode === dropModeBefore
    readonly property bool dropAfter: dragTargetMode === dropModeAfter
    readonly property bool dropAsChild: dragTargetMode === dropModeChild
    readonly property bool dropAsRoot: dragTargetMode === dropModeRoot
    readonly property bool isRootItem: !hasParentItem && treeDepth === 0
    readonly property bool isBranchItem: effectiveHasChildItems
    readonly property bool isLeafItem: !effectiveHasChildItems
    readonly property string childItemKeysText: joinedTextList(childItemKeys)
    readonly property string childItemLabelsText: joinedTextList(childItemLabels)
    readonly property string ancestorItemKeysText: joinedTextList(ancestorItemKeys)
    readonly property string ancestorLabelsText: joinedTextList(ancestorLabels)
    readonly property string pathItemKeysText: joinedTextList(pathItemKeys)
    readonly property string pathItemLabelsText: joinedTextList(pathItemLabels)
    readonly property string firstChildItemKey: {
        const keys = normalizedStringArray(childItemKeys)
        return keys.length > 0 ? keys[0] : ""
    }
    readonly property string firstChildItemLabel: {
        const labels = normalizedStringArray(childItemLabels)
        return labels.length > 0 ? labels[0] : ""
    }
    readonly property string lastChildItemKey: {
        const keys = normalizedStringArray(childItemKeys)
        return keys.length > 0 ? keys[keys.length - 1] : ""
    }
    readonly property string lastChildItemLabel: {
        const labels = normalizedStringArray(childItemLabels)
        return labels.length > 0 ? labels[labels.length - 1] : ""
    }
    readonly property bool isFirstSibling: siblingCount > 0 && siblingIndex === 0
    readonly property bool isLastSibling: siblingCount > 0 && siblingIndex === siblingCount - 1
    readonly property bool isOnlySibling: siblingCount === 1
    readonly property bool isFirstVisibleSibling: visibleSiblingCount > 0 && visibleSiblingIndex === 0
    readonly property bool isLastVisibleSibling: visibleSiblingCount > 0
        && visibleSiblingIndex === visibleSiblingCount - 1
    readonly property bool isOnlyVisibleSibling: visibleSiblingCount === 1
    readonly property bool resolvedSelected: hierarchyList ? hierarchyList.activeItem === control : selected
    readonly property string resolvedIconName: {
        const rawName = iconName === undefined || iconName === null ? "" : String(iconName)
        return rawName.trim()
    }
    readonly property url resolvedIconSource: iconSource.toString().length > 0
        ? iconSource
        : resolvedIconName.length > 0
            ? Theme.iconPath(resolvedIconName)
            : ""
    readonly property int resolvedSelectionDirection: {
        const raw = selectionDirection
        if (raw === undefined || raw === null)
            return control.expanded ? directionDown : directionRight
        if (typeof raw === "number")
            return Math.max(directionRight, Math.min(directionDown, Math.round(raw)))

        const normalized = String(raw).trim().toLowerCase()
        if (normalized.length === 0 || normalized === "auto")
            return control.expanded ? directionDown : directionRight
        if (normalized === "left")
            return directionLeft
        if (normalized === "up")
            return directionUp
        if (normalized === "down")
            return directionDown
        return control.expanded ? directionDown : directionRight
    }
    readonly property real resolvedChevronRotation: {
        if (control.resolvedSelectionDirection === control.directionLeft)
            return 90
        if (control.resolvedSelectionDirection === control.directionUp)
            return 180
        if (control.resolvedSelectionDirection === control.directionDown)
            return 0
        return -90
    }
    readonly property string resolvedChevronIconName: {
        if (control.resolvedSelectionDirection === control.directionLeft)
            return "generalchevronLeft"
        if (control.resolvedSelectionDirection === control.directionUp)
            return "generalchevronUp"
        if (control.resolvedSelectionDirection === control.directionDown)
            return "generalchevronDown"
        return "generalchevronRight"
    }
    readonly property url resolvedChevronSource: Theme.iconPath(control.resolvedChevronIconName)
    readonly property int uxState: {
        if (control.dragPreviewActive)
            return control.uxStateDrag
        if (!control.enabled || !control.activatable)
            return control.uxStateInactive
        if (control.resolvedSelected)
            return control.uxStateActive
        if (control.down)
            return control.uxStatePressed
        if (control.hovered)
            return control.uxStateHover
        return control.uxStateIdle
    }
    readonly property string uxStateName: {
        if (control.uxState === control.uxStateHover)
            return "Hover"
        if (control.uxState === control.uxStateActive)
            return "Active"
        if (control.uxState === control.uxStateInactive)
            return "Inactive"
        if (control.uxState === control.uxStatePressed)
            return "Pressed"
        if (control.uxState === control.uxStateDrag)
            return "Drag"
        return "Idle"
    }
    readonly property int interactionState: resolvedSelected
        ? stateActive
        : (enabled && activatable && hovered ? stateHover : stateIdle)
    readonly property string interactionStateName: interactionState === stateActive
        ? "Active"
        : (interactionState === stateHover ? "Hover" : "Idle")
    readonly property bool isHoverState: uxState === uxStateHover
    readonly property bool isActiveState: uxState === uxStateActive
    readonly property bool isInactiveState: uxState === uxStateInactive
    readonly property bool isPressedState: uxState === uxStatePressed
    readonly property bool isDragState: uxState === uxStateDrag
    readonly property color resolvedRowBackgroundColor: {
        if (control.isDragState)
            return control.rowBackgroundColorDrag
        if (control.isActiveState)
            return control.rowBackgroundColorActive
        if (control.isInactiveState)
            return control.rowBackgroundColorInactive
        return control.rowBackgroundColorIdle
    }
    readonly property color resolvedRowBackgroundColorHover: {
        if (control.isDragState)
            return control.rowBackgroundColorDrag
        if (control.isActiveState)
            return control.rowBackgroundColorActive
        if (control.isInactiveState)
            return control.rowBackgroundColorInactive
        return control.rowBackgroundColorHover
    }
    readonly property color resolvedRowBackgroundColorPressed: {
        if (control.isDragState)
            return control.rowBackgroundColorDrag
        if (control.isActiveState)
            return control.rowBackgroundColorActive
        if (control.isInactiveState)
            return control.rowBackgroundColorInactive
        return control.rowBackgroundColorPressed
    }
    readonly property color rowBackgroundColor: resolvedRowBackgroundColor
    readonly property real iconSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property real iconHiDpiScale: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1.0
    readonly property int iconSourceSize: Math.max(1, Math.round(control.iconSize * control.iconSupersampleScale * control.iconHiDpiScale))
    readonly property int chevronSourceSize: Math.max(1, Math.round(control.chevronSize * control.iconSupersampleScale * control.iconHiDpiScale))

    function normalizedText(value) {
        if (value === undefined || value === null)
            return ""
        return String(value)
    }

    function normalizedStringArray(value) {
        const source = Array.isArray(value) ? value : []
        const normalized = []
        for (let i = 0; i < source.length; i++) {
            const entry = source[i]
            if (entry === undefined || entry === null)
                continue
            normalized.push(String(entry))
        }
        return normalized
    }

    function joinedTextList(value) {
        return normalizedStringArray(value).join(", ")
    }

    function requestActivationFromInteraction() {
        if (!control.canBecomeActive)
            return

        if (control.hierarchyList && control.hierarchyList.requestActivate) {
            control.hierarchyList.requestActivate(control)
            return
        }

        if (!control.selected)
            control.selected = true
    }

    function pointInHierarchy(localX, localY) {
        if (!control.hierarchyList)
            return Qt.point(localX, localY)
        return control.mapToItem(control.hierarchyList, localX, localY)
    }

    function beginDrag(localX, localY) {
        if (!control.dragEnabled
                || !control.hierarchyList
                || !control.hierarchyList._beginEditableDragForItem) {
            return false
        }

        const hierarchyPoint = pointInHierarchy(localX, localY)
        return control.hierarchyList._beginEditableDragForItem(control, hierarchyPoint.x, hierarchyPoint.y)
    }

    function updateDrag(localX, localY) {
        if (!control.hierarchyList || !control.hierarchyList._updateEditableDrag)
            return false

        const hierarchyPoint = pointInHierarchy(localX, localY)
        return control.hierarchyList._updateEditableDrag(hierarchyPoint.x, hierarchyPoint.y)
    }

    function endDrag(commitMove) {
        if (!control.hierarchyList || !control.hierarchyList._endEditableDrag)
            return false
        return control.hierarchyList._endEditableDrag(commitMove)
    }

    function commitDrag() {
        return endDrag(true)
    }

    function cancelDrag() {
        return endDrag(false)
    }

    function moveTo(targetIndex, targetDepth) {
        if (!control.hierarchyList || !control.hierarchyList._applyEditableMove)
            return false
        return control.hierarchyList._applyEditableMove(control, targetIndex, targetDepth)
    }

    function moveBefore(targetItem) {
        if (!control.hierarchyList || !control.hierarchyList._applyEditableMoveByDropMode)
            return false
        return control.hierarchyList._applyEditableMoveByDropMode(control, targetItem, "before")
    }

    function moveAfter(targetItem) {
        if (!control.hierarchyList || !control.hierarchyList._applyEditableMoveByDropMode)
            return false
        return control.hierarchyList._applyEditableMoveByDropMode(control, targetItem, "after")
    }

    function moveAsChildOf(targetItem) {
        if (!control.hierarchyList || !control.hierarchyList._applyEditableMoveByDropMode)
            return false
        return control.hierarchyList._applyEditableMoveByDropMode(control, targetItem, "child")
    }

    function moveToRoot() {
        if (!control.hierarchyList || !control.hierarchyList._applyEditableMoveByDropMode)
            return false
        return control.hierarchyList._applyEditableMoveByDropMode(control, null, "root")
    }

    tone: AbstractButton.Borderless
    state: control.uxStateName
    leftPadding: computedLeftPadding
    rightPadding: rowRightPadding
    topPadding: 0
    bottomPadding: 0
    spacing: Theme.gapNone
    cornerRadius: Theme.radiusControl
    implicitWidth: itemWidth
    implicitHeight: rowHeight
    width: parent ? parent.width : implicitWidth
    visible: rowVisible
    opacity: dragPreviewActive ? dragPreviewOpacity : 1.0
    focusPolicy: control.canBecomeActive ? Qt.StrongFocus : Qt.NoFocus
    activeFocusOnTab: control.canBecomeActive

    backgroundColor: resolvedRowBackgroundColor
    backgroundColorHover: resolvedRowBackgroundColorHover
    backgroundColorPressed: resolvedRowBackgroundColorPressed
    backgroundColorDisabled: resolvedRowBackgroundColor

    onPressed: control.requestActivationFromInteraction()
    onClicked: control.requestActivationFromInteraction()

    DragHandler {
        id: itemDragHandler
        enabled: control.dragEnabled
        target: null
        acceptedButtons: Qt.LeftButton

        onActiveChanged: {
            if (active) {
                control.beginDrag(centroid.pressPosition.x, centroid.pressPosition.y)
                return
            }
            control.commitDrag()
        }
        onCentroidChanged: {
            if (!active)
                return
            control.updateDrag(centroid.position.x, centroid.position.y)
        }
        onCanceled: {
            control.cancelDrag()
        }
    }

    onHierarchyListChanged: {
        if (hierarchyList && hierarchyList.registerItem)
            hierarchyList.registerItem(control)
    }

    contentItem: Item {
        implicitWidth: Math.max(0, control.itemWidth - control.leftPadding - control.rightPadding)
        implicitHeight: control.rowHeight

        RowLayout {
            anchors.fill: parent
            spacing: control.leadingSpacing

            Item {
                id: iconSlot
                objectName: "hierarchyItemIcon"
                Layout.preferredWidth: control.iconSize
                Layout.preferredHeight: control.iconSize
                Layout.alignment: Qt.AlignVCenter

                Image {
                    id: iconImage
                    anchors.centerIn: parent
                    visible: control.iconGlyph.length === 0 && control.resolvedIconSource.toString().length > 0
                    width: control.iconSize
                    height: control.iconSize
                    source: RenderQuality.resolveTextureSource(control.resolvedIconSource)
                    sourceSize.width: control.iconSourceSize
                    sourceSize.height: control.iconSourceSize
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: RenderQuality.mipmapEnabled
                }

                Label {
                    anchors.centerIn: parent
                    visible: control.iconGlyph.length > 0
                    text: control.iconGlyph
                    style: body
                    color: control.enabled ? control.textColorNormal : control.textColorDisabled
                    font.pixelSize: control.iconSize
                    font.weight: Font.Normal
                    font.styleName: "Regular"
                    lineHeight: control.iconSize
                    lineHeightMode: Text.FixedHeight
                }

                Item {
                    anchors.centerIn: parent
                    visible: !iconImage.visible && control.iconGlyph.length === 0
                    width: 12
                    height: 12

                    Rectangle {
                        anchors.fill: parent
                        radius: 3
                        color: "transparent"
                        border.width: 1
                        border.color: control.iconPlaceholderColor
                        antialiasing: true
                    }

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 2
                        radius: 2
                        color: control.iconPlaceholderColor
                        opacity: 0.16
                        antialiasing: true
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                implicitHeight: control.rowHeight
                clip: true

                Label {
                    id: labelNode
                    objectName: "hierarchyItemLabel"
                    anchors.fill: parent
                    style: body
                    text: control.text
                    color: control.enabled ? control.textColorNormal : control.textColorDisabled
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                    lineHeight: 16
                    lineHeightMode: Text.FixedHeight
                }
            }

            Item {
                id: chevronSlot
                objectName: "hierarchyItemChevron"
                visible: control.effectiveShowChevron
                Layout.preferredWidth: control.effectiveShowChevron ? control.chevronSize : 0
                Layout.preferredHeight: control.chevronSize
                Layout.alignment: Qt.AlignVCenter

                Image {
                    anchors.fill: parent
                    source: RenderQuality.resolveTextureSource(control.resolvedChevronSource)
                    sourceSize.width: control.chevronSourceSize
                    sourceSize.height: control.chevronSourceSize
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: RenderQuality.mipmapEnabled
                    opacity: control.enabled ? 1.0 : 0.45
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: control.canToggleExpanded
                    acceptedButtons: Qt.LeftButton
                    onClicked: function(mouse) {
                        mouse.accepted = true
                        control.expanded = !control.expanded
                        control.requestActivationFromInteraction()
                    }
                }
            }
        }
    }

    onIndentLevelChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
    }
    onItemKeyChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
    }
    onTextChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
    }
    onShowChevronChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
    }
    onHasChildItemsChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
    }
    onChildCountChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
    }
    onExpandedChanged: {
        if (control.hierarchyList && control.hierarchyList.notifyExpansionChanged)
            control.hierarchyList.notifyExpansionChanged(control)
    }
    onEnabledChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
        if (!control.enabled && control.hierarchyList && control.hierarchyList.scheduleNormalizeActiveItem)
            control.hierarchyList.scheduleNormalizeActiveItem()
    }
    onActivatableChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
        if (!control.activatable && control.hierarchyList && control.hierarchyList.scheduleNormalizeActiveItem)
            control.hierarchyList.scheduleNormalizeActiveItem()
    }
    onDragPreviewActiveChanged: {
        if (control.hierarchyList && control.hierarchyList.scheduleRefreshState)
            control.hierarchyList.scheduleRefreshState()
    }

    QtObject {
        Component.onCompleted: {
            if (control.hierarchyList && control.hierarchyList.registerItem)
                control.hierarchyList.registerItem(control)
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.HierarchyItem {
//     label: "Main Camera"
//     iconName: "toolwindowhierarchy"
//     indentLevel: 1
//     showChevron: true
//     activatable: true
// }
