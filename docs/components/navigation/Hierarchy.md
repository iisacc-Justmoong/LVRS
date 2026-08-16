# Hierarchy

Location: `qml/components/navigation/Hierarchy.qml`

`Hierarchy` is a hierarchy-panel surface composed of toolbar + scrollable depth-aware hierarchy list.

## Purpose

- Render depth-array/list/model hierarchy data with explicit expand/collapse controls.
- Provide activation and expansion callbacks for host features.
- Keep tree navigation usable inside nested scroll containers.

## API

Required inputs:

- Toolbar declaration: `toolbarItems` (array model) or `toolbarButtons` (manual `ToolbarButton` children)
- List model binding: `model` / `treeModel`

Model and selection aliases:

- `model` / `treeModel`
- `modelColumn`
- `activeListItem`
- `activeListItemId`
- `activeListItemKey`

Model role aliases:

- `itemIdRole`
- `itemKeyRole`
- `labelRole`
- `iconNameRole`
- `iconSourceRole`
- `iconGlyphRole`
- `depthRole`
- `countRole`
- `enabledRole`
- `expandedRole`
- `selectedRole`
- `activatableRole`
- `draggableRole`
- `showChevronRole`

Toolbar aliases:

- `toolbarButtons`
- `toolbarItems`
- `activeToolbarButton`
- `activeToolbarButtonId`
- `activeToolbarIndex`

Behavior aliases:

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

```qml
import LVRS 1.0 as LV

LV.Hierarchy {
    model: backendHierarchyModel
    itemKeyRole: "key"
    labelRole: "name"
    depthRole: "depth"
    countRole: "counter"
}
```

## How It Works

- The Figma panel baseline (`180:1012`) is `200x530` on desktop and `400x1060` under the mobile `2x` profile, using `Theme.panelBackground05`.
- Its toolbar occupies the top `26px` (`52px` mobile). The list starts immediately below it and the sixteen-row reference content occupies `320px` (`640px` mobile), leaving the remaining panel height available to the scroll viewport.
- Toolbar and list communicate through explicit signals and forwarded aliases.
- `model`, `modelColumn`, and role aliases forward directly to the internal `HierarchyList`, so arrays, QML `ListModel`, and C++ `QAbstractItemModel` sources can be injected at the panel level.
- `ensureListItemVisible` adjusts flickable viewport when list requests visibility.
- `WheelScrollGuard` is installed to prevent nested scroll bleed.
- Optional `ListFooter` is anchored bottom-left; when visible, list viewport ends at footer top.
- `editable` enables item-owned drag/drop on generated `HierarchyItem` rows; the underlying model must expose mutable object rows with depth state, either as an object array, QML `ListModel`/list-like model, or C++ `QAbstractItemModel` with writable depth roles and row moving.
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
