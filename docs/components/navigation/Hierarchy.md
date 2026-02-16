# Hierarchy

Location: `qml/components/navigation/Hierarchy.qml`

`Hierarchy` is a tree-panel surface composed of toolbar + scrollable hierarchy list.

## Purpose

- Render nested model data with explicit expand/collapse controls.
- Provide activation and expansion callbacks for host features.
- Keep tree navigation usable inside nested scroll containers.

## API

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
- `keyboardListNavigationEnabled`

Methods:

- `expandAll()`
- `collapseAll(keepRootExpanded)`
- `activateListItemById(itemId)`
- `activateListItemByKey(itemKey)`

Signals:

- `toolbarActivated(button, buttonId, index)`
- `toolbarButtonTriggered(button, buttonId, index, item)`
- `toolbarEventTriggered(eventName, payload, index, item, buttonId)`
- `listItemActivated(item, itemId, index)`
- `listItemExpanded(item, itemId, index, expanded)`

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
            label: "Root",
            expanded: true,
            children: [{ key: "child", label: "Child" }]
        }
    ]
}
```

## How It Works

- Toolbar and list communicate through explicit signals and forwarded aliases.
- `ensureListItemVisible` adjusts flickable viewport when list requests visibility.
- `WheelScrollGuard` is installed to prevent nested scroll bleed.

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
