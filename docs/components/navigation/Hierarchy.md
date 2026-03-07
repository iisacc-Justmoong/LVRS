# Hierarchy

Location: `qml/components/navigation/Hierarchy.qml`

`Hierarchy` is a tree-panel surface composed of toolbar + scrollable hierarchy list.

## Purpose

- Render nested model data with explicit expand/collapse controls.
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

- `autoExpandDepth`
- `depthRole`
- `inferDepthFromStructure`
- `keyboardListNavigationEnabled`
- `editable`

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
    toolbarItems: [
        { id: "structure", iconName: "projectStructure", eventName: "hierarchy.structure" },
        { id: "layers", iconName: "projectStructure", events: ["hierarchy.layers", "analytics.layers"] }
    ]
    model: [
        {
            key: "root",
            depth: 0,
            label: "Root",
            expanded: true,
            children: [{ key: "child", depth: 1, label: "Child" }]
        }
    ]
    footerVisible: true
    footerButton1: ({ type: "icon", iconName: "projectStructure" })
    footerButton2: ({ type: "icon", iconName: "delete" })
    footerButton3: ({ type: "menu", iconName: "viewMoreSymbolicDefault" })
}
```

## How It Works

- Toolbar and list communicate through explicit signals and forwarded aliases.
- `ensureListItemVisible` adjusts flickable viewport when list requests visibility.
- `WheelScrollGuard` is installed to prevent nested scroll bleed.
- Optional `ListFooter` is anchored bottom-left; when visible, list viewport ends at footer top.
- `editable` forwards `HierarchyList` drag-depth editing; the underlying model must be an array-backed object tree.

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
