# HierarchyList

Location: `qml/components/navigation/HierarchyList.qml`

`HierarchyList` is a depth-aware view list that renders `HierarchyItem` rows from either manual children or a flat model array with explicit depth values.

## Purpose

- Render a depth-array model into visible hierarchy rows.
- Maintain activation, visibility, and expansion state efficiently.
- Provide keyboard navigation and ancestor auto-expansion behavior.

## Core API

Model and roles:

- `model` (main tree input), `treeModel` (compat alias)
- `itemIdRole`, `itemKeyRole`, `labelRole`, `iconNameRole`, `iconSourceRole`, `iconGlyphRole`
- `enabledRole`, `expandedRole`, `selectedRole`, `activatableRole`, `draggableRole`, `showChevronRole`
- `depthRole` (default `depth`)

Generated row defaults:

- `generatedIndentStep` (default `8`), `generatedRowHeight` (default `20`), `generatedItemWidth` (default `200`)
- `generatedIconSize` (default `16`), `generatedChevronSize` (default `16`)

State:

- `activeItem`, `activeItemId`, `activeItemKey`
- `itemCount`, `visibleItemCount` (readonly)
- `keyboardNavigationEnabled`
- `autoExpandAncestorsOnActivate`
- `editable` (enables drag-based depth editing for array-backed object depth models)

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
- tree relations: `parentItem(item)`, `firstChildItem(item)`

## Behavior Contract

- `model` is present: list generates managed `HierarchyItem` instances.
- `model` is empty: uses manually slotted `items` as managed rows.
- Model input is expected to be a flat array/list of rows with explicit depth data (`indentLevel` first, then `depthRole`).
- Child presence is inferred by indent/order and synchronized into each row `hasChildItems`.
- Generated row defaults mirror `HierarchyItem` defaults unless explicitly overridden on the list.
- Managed rows are enriched with item metadata on every refresh: `parentItemKey`, `parentLabel`, `parentPathLabel`, `pathLabel`, `ancestorItemKeys`, `ancestorLabels`, `pathItemKeys`, `pathItemLabels`, `childCount`, `visibleChildCount`, `descendantCount`, `visibleDescendantCount`, `childItemKeys`, `childItemLabels`, `flatIndex`, `visibleIndex`, `siblingIndex`, `visibleSiblingIndex`, `siblingCount`, `visibleSiblingCount`.
- Visibility is computed from ancestor expansion state and cached incrementally.
- Activation can auto-expand ancestors and requests viewport alignment via `ensureVisibleRequested`.
- User interaction can re-emit `activeChanged` for the already-active row, so host behaviors can bind actions to deliberate repeat taps/clicks without mutating selection.
- Generated rows can consume per-node activation affordance through `activatableRole` (default `activatable`, with `selectable` fallback), and non-activatable rows are excluded from activation normalization and keyboard activation targets.
- Generated rows can consume per-node drag affordance through `draggableRole` (default `draggable`, with `dragAllowed` fallback), so editable lists can keep selected rows interactive while locking specific nodes against drag startup.
- `editable` currently supports only array-backed object depth models; `ListModel` and primitive-only arrays are not editable.
- `editable` does not expose the drag API on the list itself; it only enables the item-level drag/drop contract on generated `HierarchyItem` rows.
- Generated editable rows keep desktop drag immediate, but mobile-target pointer drag starts only after a `1000ms` long press so touch scrolling stays with the surrounding `Flickable` until the hold gate is met.
- Mobile-target row activation is committed on release/click rather than press, so list scrolling can claim the gesture before `activeItem` changes.
- Depth reorder operations update the flat backing array and rewrite each moved row's depth plus `parentKey` / `parentItemKey` fields.

## Usage

```qml
import LVRS 1.0 as LV

LV.HierarchyList {
    model: [
        { key: "root", depth: 0, label: "Root", expanded: true },
        { key: "child", depth: 1, label: "Child" }
    ]
}
```
