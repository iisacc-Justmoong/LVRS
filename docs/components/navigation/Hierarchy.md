# Hierarchy

Location: `qml/components/navigation/Hierarchy.qml`

`Hierarchy` is a hierarchy-panel surface composed of toolbar + scrollable depth-aware hierarchy list.

## Purpose

- Render depth-array hierarchy data with explicit expand/collapse controls.
- Provide activation and expansion callbacks for host features.
- Keep tree navigation usable inside nested scroll containers.

## API

Required inputs:

- Toolbar declaration: `toolbarItems` (array model) or `toolbarButtons` (manual `ToolbarButton` children)
- List model binding: `model` / `treeModel`

Model and selection aliases:

- `model` / `treeModel`
- `activeListItem`
- `activeListItemId`
- `activeListItemKey`

Toolbar aliases:

- `toolbarButtons`
- `toolbarItems`
- `activeToolbarButton`
- `activeToolbarButtonId`
- `activeToolbarIndex`

Behavior aliases:

- `depthRole`
- `draggableRole`
- `countRole`
- `keyboardListNavigationEnabled`
- `editable`
- list scroll physics: `listOvershootEnabled`, `listFlickDeceleration`, `listMaximumFlickVelocity`, `listReboundDuration`
- readonly list viewport policy: `listBoundsBehavior`, `listBoundsMovement`

Optional footer:

- `footerVisible`
- `footerInteractive`
- `footerButton1`
- `footerButton2`
- `footerButton3`

Methods:

- `expandAll()`
- `collapseAll(keepRootExpanded)`
- `activateListItemById(itemId)`
- `activateListItemByKey(itemKey)`
- `triggerFooterButton(index)`

Signals:

- `toolbarActivated(button, buttonId, index)`
- `toolbarButtonTriggered(button, buttonId, index, item)`
- `toolbarEventTriggered(eventName, payload, index, item, buttonId)`
- `listItemActivated(item, itemId, index)`
- `listItemExpanded(item, itemId, index, expanded)`
- `listItemMoved(item, itemId, itemKey, fromIndex, toIndex, depth)`
- `footerButtonTriggered(index, config)`

## Usage

```qml
import LVRS 1.0 as LV

LV.Hierarchy {
    countRole: "counter"
    toolbarItems: [
        { id: "structure", iconName: "projectStructure", eventName: "hierarchy.structure" },
        { id: "layers", iconName: "projectStructure", events: ["hierarchy.layers", "analytics.layers"] }
    ]
    model: [
        { key: "root", depth: 0, label: "Root", expanded: true, counter: 2 },
        { key: "child", depth: 1, label: "Child", counter: 7 }
    ]
    footerVisible: true
    footerButton1: ({ type: "icon", iconName: "projectStructure" })
    footerButton2: ({ type: "icon", iconName: "delete" })
    footerButton3: ({ type: "menu", iconName: "viewMoreSymbolicDefault" })
}
```

## How It Works

- Toolbar and list communicate through explicit signals and forwarded aliases.
- `countRole` forwards directly to the internal `HierarchyList`, so model-backed tree counters can be enabled from the panel wrapper.
- `ensureListItemVisible` adjusts flickable viewport when list requests visibility.
- `WheelScrollGuard` is installed to prevent nested scroll bleed.
- Optional `ListFooter` is anchored bottom-left; when visible, list viewport ends at footer top.
- `editable` enables item-owned drag/drop on generated `HierarchyItem` rows; the underlying model must be an array-backed object depth list.
- In mobile-target runs, editable row drag requires a `1000ms` long press before `HierarchyItem` enters drag/drop mode, which preserves touch scrolling priority inside the panel `Flickable`.
- In mobile-target runs, row activation also commits on release/click instead of press, so a vertical drag can still be claimed by the list scroll path before the active row changes.
- Mobile-target list scrolling now enables overshoot + rebound (`DragAndOvershootBounds`/`FollowBoundsBehavior`) and tuned flick momentum (`listFlickDeceleration`, `listMaximumFlickVelocity`) for iOS-like inertial feel at edges.
- `listItemActivated` mirrors deliberate repeat activation gestures on the already-active row, so panel hosts can treat a second tap/click as an action trigger without changing selection.

## Advanced Usage: Programmatic Activation

```qml
import LVRS 1.0 as LV

LV.Hierarchy {
    id: tree
}

function focusNodeByKey(key) {
    tree.activateListItemByKey(key)
}
```

## Operational Notes

- Keep item ids/keys stable for reliable programmatic activation.
- Combine `expandAll()` with `activate*()` in onboarding flows to reveal deep nodes deterministically.

## Failure Pattern

Using non-unique keys for sibling nodes breaks programmatic activation and expansion tracking.
Assign stable unique identifiers for each logical node.
