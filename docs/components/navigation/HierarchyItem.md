# HierarchyItem

Location: `qml/components/navigation/HierarchyItem.qml`

`HierarchyItem` is the smallest hierarchy row primitive used by `HierarchyList`, but it now exposes enough structural and UX state to act as a real directory-tree, outliner, or hierarchy node contract.

## Purpose

- Render the Figma-aligned row baseline: icon, label, optional chevron.
- Expose per-item hierarchy metadata directly on the row itself.
- Expose UX-state and affordance state directly on the row itself.

## Implemented API Inventory

Identity and list context:

- `itemId`, `itemKey`, `parentItemKey`
- `parentLabel`
- `pathLabel`, `parentPathLabel`
- `hierarchyList`
- `nodeData`
- `generatedByTreeModel`
- `resolvedItemId` (readonly fallback using `flatIndex`)
- `resolvedItemKey` (readonly fallback using `itemId` or `flatIndex`)
- `resolvedLabel`, `resolvedPathLabel` (readonly)

Tree structure and ordering:

- `indentLevel`, `treeDepth`
- `flatIndex`
- `visibleIndex`
- `siblingIndex`
- `visibleSiblingIndex`
- `hasParentItem` (readonly)
- `isRootItem`, `isBranchItem`, `isLeafItem` (readonly)

Child metadata:

- `hasChildItems`
- `childCount`
- `visibleChildCount`
- `hiddenChildCount` (readonly)
- `descendantCount`
- `visibleDescendantCount`
- `hiddenDescendantCount` (readonly)
- `childItemKeys`
- `childItemLabels`
- `childItemKeysText` (readonly comma-separated string)
- `childItemLabelsText` (readonly comma-separated string)
- `hasVisibleChildItems`, `hasHiddenChildItems` (readonly)
- `hasVisibleDescendants`, `hasHiddenDescendants` (readonly)
- `firstChildItemKey`, `firstChildItemLabel` (readonly)
- `lastChildItemKey`, `lastChildItemLabel` (readonly)

Ancestor and path metadata:

- `ancestorItemKeys`
- `ancestorLabels`
- `ancestorItemKeysText` (readonly comma-separated string)
- `ancestorLabelsText` (readonly comma-separated string)
- `pathItemKeys`
- `pathItemLabels`
- `pathItemKeysText` (readonly comma-separated string)
- `pathItemLabelsText` (readonly comma-separated string)

Sibling metadata:

- `siblingIndex`
- `visibleSiblingIndex`
- `siblingCount`
- `visibleSiblingCount`
- `isFirstSibling`, `isLastSibling`, `isOnlySibling` (readonly)
- `isFirstVisibleSibling`, `isLastVisibleSibling`, `isOnlyVisibleSibling` (readonly)

Expansion and chevron affordance:

- `showChevron`
- `effectiveShowChevron` (readonly)
- `chevronExpandable` (readonly)
- `expanded`
- `collapsed` (readonly)
- `canToggleExpanded`, `canExpand`, `canCollapse` (readonly)
- direction constants: `directionRight`, `directionLeft`, `directionUp`, `directionDown`
- `selectionDirection`
- `resolvedSelectionDirection` (readonly)
- `resolvedChevronRotation` (readonly)
- `resolvedChevronIconName` (readonly)

Activation and selection:

- `selected`
- `activatable`
- `selectable` (alias of `activatable`)
- `active` (readonly)
- `inactive` (readonly)
- `canBecomeActive` (readonly)

UX state:

- enum constants:
  `uxStateIdle`, `uxStateHover`, `uxStateActive`, `uxStateInactive`, `uxStatePressed`, `uxStateDrag`
- `uxState` (readonly)
- `uxStateName` (readonly)
- compatibility state fields:
  `stateIdle`, `stateHover`, `stateActive`
- compatibility interaction fields:
  `interactionState`, `interactionStateName`
- state flags:
  `isHoverState`, `isActiveState`, `isInactiveState`, `isPressedState`, `isDragState`

Drag affordance:

- `dragAllowed`
- `dragPreviewActive`
- `dragPreviewOpacity`
- `draggable` (readonly; reflects list editability)
- `dragEnabled` (readonly; item-level drag API availability)
- `pointerDragRequiresLongPress` (readonly)
- `immediatePointerDragEnabled` (readonly)
- `mobileDragHoldInterval` (default `1000`)
- `dragging` (readonly)
- `dragActive`, `dragTargetValid`, `dragSourceItem`, `dragAnchorItem`, `dragParentTargetItem` (readonly)
- `dragSourceIndex`, `dragSourceEndIndex` (readonly)
- `dragTargetIndex`, `dragTargetDepth` (readonly)
- drop mode constants:
  `dropModeNone`, `dropModeBefore`, `dropModeAfter`, `dropModeChild`, `dropModeRoot`
- `dragTargetMode`, `dragTargetModeName` (readonly)
- `dragTargetParentItemKey`, `dragTargetParentLabel`, `dragTargetParentPathLabel` (readonly)
- `dragTargetAnchorItemKey`, `dragTargetAnchorLabel` (readonly)
- `dropBefore`, `dropAfter`, `dropAsChild`, `dropAsRoot` (readonly)

Drag/move methods:

- `beginDrag(localX, localY)`
- `updateDrag(localX, localY)`
- `endDrag(commitMove)`
- `commitDrag()`
- `cancelDrag()`
- `moveTo(targetIndex, targetDepth)`
- `moveBefore(targetItem)`
- `moveAfter(targetItem)`
- `moveAsChildOf(targetItem)`
- `moveToRoot()`

Drag signals:

- `dragStarted(sourceIndex, sourceEndIndex, sourceDepth)`
- `dragUpdated(targetIndex, targetDepth, modeName, parentItemKey, anchorItemKey)`
- `dragEnded(committed, fromIndex, toIndex, targetDepth, modeName, parentItemKey, anchorItemKey)`

Layout defaults from Figma:

- `rowHeight` (default `20`)
- `itemWidth` (default `200`)
- `iconSize` (default `16`)
- `chevronSize` (default `16`)
- `baseLeftPadding` (default `8`)
- `rowRightPadding` (default `8`)
- `leadingSpacing` (default `2`)
- `computedLeftPadding` (readonly)

Visual tokens:

- `iconPlaceholderColor`
- `textColorNormal`, `textColorDisabled`
- `rowBackgroundColorIdle`
- `rowBackgroundColor` (readonly resolved compatibility color)
- `rowBackgroundColorHover`
- `rowBackgroundColorPressed`
- `rowBackgroundColorActive`
- `rowBackgroundColorInactive`
- `rowBackgroundColorDrag`
- `rowVisible` (readonly, from `_rowVisibleInternal`)

## Behavior Contract

- The default row matches the Figma baseline: `200x20`, `8px` horizontal padding, `2px` leading gap, `16px` icon and chevron.
- The detailed Figma component (`314:93`) defines three canonical visual fills: `Default` = transparent, `Inactive` = `Theme.panelBackground12`, `Active` = `Theme.accentBlueMuted`.
- `HierarchyList` synchronizes parent/child/order/path metadata onto each managed row.
- Item-level drag/drop is initiated and committed from `HierarchyItem`; `HierarchyList` only supplies the backing projection and array rewrite.
- `childCount` is the immediate child count; `descendantCount` is the full subtree count below the row.
- `visibleDescendantCount` counts only currently visible descendants under the row, so collapsed descendants remain measurable through `hiddenDescendantCount`.
- `childItemKeysText` and `childItemLabelsText` provide the string-form child summary requested by consumers that do not want to inspect arrays directly.
- `ancestor*`, `pathItem*`, and sibling-count fields expose lineage and local ordering directly on the row without an additional lookup back into `HierarchyList`.
- Dragging a generated row rewrites the connected flat depth-array model in place: row order changes, moved subtree depth changes, and `parentKey` / `parentItemKey` are recomputed on the backing objects.
- `dragAllowed` lets hosts keep a row selectable and visible while preventing drag startup for protected nodes inside an otherwise editable hierarchy.
- On mobile targets (`Theme.mobileTarget == true`), pointer drag startup is delayed until the row is held for `1000ms`; desktop targets keep immediate drag pickup.
- On mobile targets, touch activation is committed on release/click instead of press, so surrounding `Flickable` surfaces can steal the gesture for scrolling before the row becomes active.
- `dragTargetModeName` resolves the current drop intent as `before`, `after`, `child`, or `root`.
- `activatable`/`selectable` control whether row click can make the item active without preventing chevron-driven expansion.
- `uxState` is the primary enum for UX handling. Priority is: `Drag` -> `Inactive` -> `Active` -> `Pressed` -> `Hover` -> `Idle`.
- `indentStep` defaults to `8`, so each additional depth level increases left padding by `8`.
- Row click requests activation only; row click does not toggle expansion.
- Chevron click toggles `expanded` and then requests activation when activation is allowed.
- `selectionDirection: "auto"` maps `expanded=false` to right and `expanded=true` to down.

## Usage

```qml
import LVRS 1.0 as LV

LV.HierarchyItem {
    label: "Camera"
    iconName: "toolwindowhierarchy"
    indentLevel: 1
    showChevron: true
    activatable: true
    onDragEnded: function(committed, fromIndex, toIndex, targetDepth, modeName, parentItemKey) {
        if (committed)
            console.log("moved", fromIndex, toIndex, targetDepth, modeName, parentItemKey)
    }
}
```
