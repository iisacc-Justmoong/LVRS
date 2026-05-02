# HierarchyModel

Location: `backend/model/hierarchymodel.h`, `backend/model/hierarchymodel.cpp`

`HierarchyModel` is the C++ projection layer used by `HierarchyList`.

## Purpose

- Convert an injected flat model into generated hierarchy row descriptors.
- Keep role fallback, type coercion, and `QAbstractItemModel` reading out of `HierarchyList.qml`.
- Rebuild descriptors when source model rows or role mapping properties change.

## API

Input:

- `source`
- `column`
- `itemIdRole`, `itemKeyRole`, `labelRole`
- `iconNameRole`, `iconSourceRole`, `iconGlyphRole`
- `countRole`, `enabledRole`, `expandedRole`, `selectedRole`
- `activatableRole`, `draggableRole`, `showChevronRole`, `depthRole`

Readonly:

- `descriptors`
- `count`
- `revision`
- `hasSource`

Methods:

- `descriptorAt(index)`
- `roleValue(entry, roleName, fallbackValue)`
- `depthArraySupportsEditing(nodes)`
- `sourceSupportsEditing()`
- `projectInteractionState(items)`
- `descendantRangeEnd(items, itemIndex)`
- `resolveDragTarget(items, sourceStart, sourceEnd, rawInsertionIndex, localX, indentStep, basePadding)`
- `moveDescriptors(items, sourceStart, sourceEnd, targetIndex, targetDepth)`
- `moveSourceRows(sourceStart, sourceEnd, targetIndex, targetDepth)`
- `invalidate()`

## Descriptor Contract

Each descriptor contains:

- `itemId`, `itemKey`, `parentItemKey`
- `label`, `iconName`, `iconSource`, `iconGlyph`
- `count`, `showChevron`, `hasChildren`
- `expanded`, `selected`, `enabled`, `activatable`, `draggable`
- `indentLevel`, `pathLabel`, `nodeData`

## How It Works

- Input rows are read through `ModelSource`.
- Label fallback order is `labelRole`, `text`, `title`, `name`, `display`, then `edit`.
- `indentLevel` wins over `depthRole`; missing depth defaults to `0`.
- `itemKey` uses the explicit key role, then numeric `itemId`, then row index.
- `activatableRole` falls back through `selectable`/`activatable`; `draggableRole` falls back through `dragAllowed`/`draggable`.
- `projectInteractionState` owns visibility, visible indices, lookup maps, parent/path metadata, sibling counts, child counts, and descendant counts.
- Path arrays are returned as nested `QVariantList` values so QML rows receive `ancestorItemKeys`, `pathItemKeys`, and `pathItemLabels` as arrays, not flattened strings.
- `resolveDragTarget` applies source-block removal, insertion index adjustment, depth bounds, parent lookup, and drop mode derivation.
- `moveDescriptors` returns reordered descriptors plus moved/drop metadata for editable depth models.
- `moveSourceRows` applies the same move to a direct backing model when the source is a mutable `QAbstractItemModel` or a list-like object exposed to C++. It calls `moveRows` for item models, `move(...)` for list-like models, then writes depth and parent key roles back through `setData`/`setProperty`.

## QML Boundary

`HierarchyList.qml` still creates/destroys visual `HierarchyItem` instances and forwards pointer/keyboard events. `HierarchyModel` owns descriptor projection, metadata projection, editable-model eligibility checks, descendant range calculation, drag target calculation, descriptor reordering, and direct model writes for editable model sources.
