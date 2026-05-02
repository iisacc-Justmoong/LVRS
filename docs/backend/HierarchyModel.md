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
