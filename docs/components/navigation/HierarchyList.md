# HierarchyList

Location: `qml/components/navigation/HierarchyList.qml`

`HierarchyList` is a tree/list manager that renders `HierarchyItem` rows from either manual children or model-driven flattened tree data.

## Purpose

- Flatten nested tree model into visible hierarchy rows.
- Maintain activation, visibility, and expansion state efficiently.
- Provide keyboard navigation and ancestor auto-expansion behavior.

## Core API

Model and roles:

- `model` (main tree input), `treeModel` (compat alias)
- `childrenRole`, `itemIdRole`, `itemKeyRole`, `labelRole`, `iconNameRole`, `iconSourceRole`, `iconGlyphRole`
- `enabledRole`, `expandedRole`, `selectedRole`, `showChevronRole`
- `autoExpandDepth`

Generated row defaults:

- `generatedIndentStep`, `generatedRowHeight`, `generatedItemWidth`
- `generatedIconSize`, `generatedChevronSize`

State:

- `activeItem`, `activeItemId`, `activeItemKey`
- `itemCount`, `visibleItemCount` (readonly)
- `keyboardNavigationEnabled`
- `autoExpandAncestorsOnActivate`

Composition:

- `default property alias items` (manual row mode)

Signals:

- `activeChanged(item, itemId, index)`
- `expansionChanged(item, expanded, index)`
- `ensureVisibleRequested(y, height)`

Primary methods:

- activation: `requestActivate(item)`, `activateById(itemId)`, `activateByKey(itemKey)`
- expansion: `expandAll()`, `collapseAll(keepRootExpanded)`
- navigation: `navigateLeft()`, `navigateRight()`, `activateRelativeVisible(step)`
- lookup helpers: `resolveById(...)`, `resolveByKey(...)`, `indexOfItem(...)`, `isItemVisible(...)`

## Behavior Contract

- `model` is present: list generates managed `HierarchyItem` instances.
- `model` is empty: uses manually slotted `items` as managed rows.
- Child presence is inferred by indent/order and synchronized into each row `hasChildItems`.
- Visibility is computed from ancestor expansion state and cached incrementally.
- Activation can auto-expand ancestors and requests viewport alignment via `ensureVisibleRequested`.

## Usage

```qml
import LVRS 1.0 as LV

LV.HierarchyList {
    model: [
        {
            key: "root",
            label: "Root",
            children: [
                { key: "child", label: "Child" }
            ]
        }
    ]
}
```
