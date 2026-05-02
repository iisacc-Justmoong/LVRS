# TableModel

Location: `backend/model/tablemodel.h`, `backend/model/tablemodel.cpp`

`TableModel` owns the non-visual table model behavior for `Table.qml`.

## Purpose

- Resolve headers, rows, visible cells, cell spans, and column types in C++.
- Own runtime cell merge/split and row/column insert/delete mutations.
- Own table geometry, row/column resize state, and cell-context action descriptors.
- Validate and coerce body cell values from header-declared types.

## API

Input:

- `rows`
- `headerCellItems`
- `headerColumns`
- `defaultHeaderText`
- `defaultCellText`
- `inputable`
- `tableWidth`
- `rowHeight`
- `cellWidth`
- `columnWidths`
- `rowHeights`
- `minColumnWidth`
- `minRowHeight`

Readonly:

- `rowCount`
- `headerCount`
- `columnCount`
- `rowsModelBacked`
- `cellEditingAvailable`
- `structureMutationAvailable`
- `resizingColumnIndex`, `resizingRowIndex`
- `contextRowIndex`, `contextColumnIndex`
- `revision`
- `undoStack`
- `canUndo`, `canRedo`
- `undoDepth`, `redoDepth`

Methods:

- data: `resolvedHeaderSource()`, `rowAt(index)`, `cellAt(rowIndex, columnIndex)`, `headerAt(columnIndex)`
- typing: `headerCellType(columnIndex)`, `columnType(columnIndex)`, `coerceCellValue(value, valueType)`, `validateCellInput(...)`
- editing: `setCellValue(rowIndex, columnIndex, value)`
- spans: `cellRowSpan(...)`, `cellColumnSpan(...)`, `mergeAnchorForCell(...)`, `isCoveredCell(...)`, `visibleCells()`
- geometry: `columnWidth(...)`, `columnX(...)`, `columnSpanWidth(...)`, `rowHeightAt(...)`, `rowY(...)`, `rowSpanHeight(...)`, `totalBodyHeight()`
- resizing: `setColumnWidth(...)`, `setRowHeight(...)`, `beginColumnResize(...)`, `updateColumnResize(...)`, `endColumnResize()`
- structure: `insertRow(...)`, `appendRow()`, `deleteRow(...)`, `removeRow(...)`, `insertColumn(...)`, `appendColumn()`, `deleteColumn(...)`, `removeColumn(...)`
- context: `setContextCell(...)`, `contextMenuDescriptors(...)`, `triggerContextAction(...)`
- merge/split: `canMergeCells(...)`, `mergeCells(...)`, `splitCell(...)`
- inputability: `rowInputable(rowEntry)`, `cellInputable(rowIndex, columnIndex)`
- history: `undo()`, `redo()`, `clearUndoStack()`

## How It Works

- Header source resolves from `headerCellItems` first, then `headerColumns`.
- When `rows` is a `QAbstractItemModel`, `TableModel` keeps the C++ model as the source of truth and builds render rows from `rowCount()`, `columnCount()`, `data(index, Qt::DisplayRole)`, and `data(index, Qt::EditRole)`.
- If default table headers are still in use, a C++ item model's horizontal `headerData(..., Qt::DisplayRole)` supplies the visible header labels.
- Header objects may declare `type`, `valueType`, `cellType`, or `dataType`.
- Primitive headers infer `string`, `int`, `float`, or `bool` from their value.
- `visibleCells()` returns a flattened render model with `x`, `y`, `width`, and `height`, and excludes covered merge members.
- Arrays and list-like objects can be displayed, including objects with `count`/`get(index)`.
- C++ item models can be displayed directly by assigning the model object to `rows`.
- `setCellValue(rowIndex, columnIndex, value)` writes C++ item-model rows through the standard `QAbstractItemModel::setData(index, coercedValue, Qt::EditRole)` path. If the model exposes named `value`, `edit`, or `text` roles, those roles are attempted after `Qt::EditRole`.
- C++ item-model edits are source-owned. The model should emit `dataChanged`, `rowsInserted`, `rowsRemoved`, `columnsInserted`, `columnsRemoved`, `layoutChanged`, `modelReset`, or `headerDataChanged` as appropriate; `TableModel` listens to these and refreshes its render descriptors.
- Structural mutations are accepted only when rows came from a mutable array/list value; read-only list-like objects stay display-only.
- Row/column structure editing and cell merge/split still require mutable array rows. C++ models own their own structural editing API.
- Before structural changes, merge metadata is normalized so old spans do not point to the wrong row or column.
- Row/column insert and delete operations keep `rowHeights` and `columnWidths` aligned with the edited structure.
- Resize operations are backend mutations and use the same undo stack as data and structure edits.
- Cell context menus are backend-described. QML only maps descriptors to visible `ContextMenu` items and forwards the selected action.
- Before each accepted array-backed mutation, `TableModel` records a snapshot in `ModelUndoStack`.
- `undo()` and `redo()` restore `rows`, the retained C++ source object when present, `headerCellItems`, `headerColumns`, `columnWidths`, `rowHeights`, and row mutability state, then emit the same model/header/geometry change signals used by normal mutations.

## QML Boundary

`Table.qml` is now a render and event adapter. It binds visual inputs into `TableModel`, renders `visibleCells()`, forwards pointer events from resize handles and cell context menus, and emits the public QML signals after backend mutations succeed. Geometry, resizing, structure editing, merge/split, value validation, context-menu availability, and undo/redo are owned by `TableModel`.
