# ModelSource

Location: `backend/model/modelsource.h`, `backend/model/modelsource.cpp`

`ModelSource` is the shared C++ reader for model-bearing QML components.

## Purpose

- Accept JavaScript arrays, primitive arrays, QML list-like objects, and `QAbstractItemModel` instances through one injected `source`.
- Project item-model rows into role-name maps without QML-side adapter logic.
- Emit revision/count changes when the backing C++ item model changes.

## API

Properties:

- `source`
- `column`
- `count` (readonly)
- `revision` (readonly)
- `itemModel` (readonly)

Methods:

- `at(index)`
- `row(index)`
- `roleValue(entry, roleName, fallbackValue)`
- `textValue(entry, roleNames, fallbackValue)`
- `boolValue(entry, roleName, fallbackValue)`
- `intValue(entry, roleName, fallbackValue)`
- `invalidate()`

## How It Works

- `QAbstractItemModel` input is read at the requested `column` and converted into a map keyed by `roleNames()`.
- `display`, `edit`, `index`, `row`, and `column` are added as stable fallback fields.
- JS/list-like inputs are read by index, `get(index)`, or `at(index)` where available.
- `rowsInserted`, `rowsRemoved`, `rowsMoved`, `modelReset`, `layoutChanged`, and `dataChanged` all invalidate `revision`.

## Consumers

- `List` uses `ModelSource` for row count, role lookup, label resolution, enabled state, and selection state.
- `HierarchyModel` uses `ModelSource` before projecting tree row descriptors.
