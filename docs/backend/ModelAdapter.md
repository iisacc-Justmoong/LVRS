# ModelAdapter

Location: `backend/navigation/modeladapter.h`, `backend/navigation/modeladapter.cpp`

`ModelAdapter` is a QML singleton used by list and hierarchy components to read C++ `QAbstractItemModel` rows through role names.

## Purpose

- Detect whether a QML `var` holds a `QAbstractItemModel`.
- Expose row count to QML.
- Convert a row into a `QVariantMap` keyed by role name.

## API

- `isItemModel(model) -> bool`
- `count(model) -> int`
- `row(model, row, column = 0) -> object`

## Behavior Contract

- `count()` returns top-level row count.
- `row()` reads the requested top-level row and column, then inserts all valid `roleNames()` values into the returned map.
- `row()` also inserts `display` and `edit` fallbacks from `Qt::DisplayRole` / `Qt::EditRole` when those keys are absent.
- The returned map includes `index`, `row`, and `column`.
- Invalid or non-model inputs return `false`, `0`, or an empty map rather than throwing.

## Usage

```qml
import LVRS 1.0 as LV

const count = LV.ModelAdapter.count(backendModel)
const first = LV.ModelAdapter.row(backendModel, 0)
```

`LV.List`, `LV.HierarchyList`, and `LV.Hierarchy` use this adapter internally when their `model` property receives a `QAbstractItemModel`.
