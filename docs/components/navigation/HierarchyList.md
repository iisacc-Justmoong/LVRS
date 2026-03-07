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
- `depthRole` (default `depth`)
- `inferDepthFromStructure` (default `false`)
- `autoExpandDepth`

Generated row defaults:

- `generatedIndentStep` (default `8`), `generatedRowHeight`, `generatedItemWidth`
- `generatedIconSize`, `generatedChevronSize`

State:

- `activeItem`, `activeItemId`, `activeItemKey`
- `itemCount`, `visibleItemCount` (readonly)
- `keyboardNavigationEnabled`
- `autoExpandAncestorsOnActivate`
- `editable` (enables drag-based depth editing for array-backed object tree models)

Composition:

- `default property alias items` (manual row mode)

Signals:

- `activeChanged(item, itemId, index)`
- `expansionChanged(item, expanded, index)`
- `ensureVisibleRequested(y, height)`
- `itemMoved(item, itemId, itemKey, fromIndex, toIndex, depth)`

Primary methods:

- activation: `requestActivate(item)`, `activateById(itemId)`, `activateByKey(itemKey)`
- expansion: `expandAll()`, `collapseAll(keepRootExpanded)`
- navigation: `navigateLeft()`, `navigateRight()`, `activateRelativeVisible(step)`
- lookup helpers: `resolveById(...)`, `resolveByKey(...)`, `indexOfItem(...)`, `isItemVisible(...)`

## Behavior Contract

- `model` is present: list generates managed `HierarchyItem` instances.
- `model` is empty: uses manually slotted `items` as managed rows.
- Indentation uses explicit model depth (`indentLevel` first, then `depthRole`); structural inference is optional via `inferDepthFromStructure`.
- Child presence is inferred by indent/order and synchronized into each row `hasChildItems`.
- Visibility is computed from ancestor expansion state and cached incrementally.
- Activation can auto-expand ancestors and requests viewport alignment via `ensureVisibleRequested`.
- `editable` currently supports only array-backed object tree models; `ListModel` and primitive-only arrays are not editable.
- While `editable` is enabled, nested `children` structure is also treated as depth input even when explicit depth fields are absent, so drag depth edits remain visually coherent.

## Usage

```qml
import LVRS 1.0 as LV

LV.HierarchyList {
    model: [
        {
            key: "root",
            depth: 0,
            label: "Root",
            children: [
                { key: "child", depth: 1, label: "Child" }
            ]
        }
    ]
}
```
