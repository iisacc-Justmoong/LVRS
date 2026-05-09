# Table

Location: `qml/components/control/display/Table.qml`

`Table` composes `TableHeader` and positioned `TableCellItem` delegates for compact data display. Its table behavior is backed by C++ `TableModel`; QML remains the render and event adapter.

## Purpose

- Provide fixed-height, dense tabular display.
- Accept array/list-model style inputs for headers and rows.
- Support cell merge/split behavior through span metadata and table-level mutation methods.

## API

Data:

- `headerCellItems` (preferred)
- `headerColumns`
- `rows`
- `headerCellDelegate`: optional component forwarded to `TableHeader.cellDelegate`.
- `cellDelegate`: optional per-body-cell component delegate. The default delegate is `TableCellItem`.

Layout:

- `rowHeight`
- `cellWidth` (`0` means auto width)
- `columnWidths` (per-column explicit widths)
- `rowHeights` (per-row explicit heights)
- `minColumnWidth`, `minRowHeight`
- `resizeHandlesVisible` (default `true`)
- `columnResizeHandleWidth`, `rowResizeHandleHeight`
- `resizingColumnIndex`, `resizingRowIndex`
- `resolvedColumnCount` (readonly)
- `rowsModelBacked` (readonly; true when `rows` is a C++ `QAbstractItemModel`)
- `cellEditingAvailable` (readonly; true when a model-backed or array-backed cell edit path exists)
- `visibleCellItems` (readonly flattened render model)
- `canUndo`, `canRedo` (readonly)
- `undoDepth`, `redoDepth` (readonly)

Visual:

- `backgroundColor`
- `borderColor`, `borderWidth`
- `headerTextColor`
- `cellTextColor`
- `dividerColor` (legacy alias for row divider baseline)
- `rowDividerColor`
- `headerSeparatorColor`
- `inputable` (default `false`; table-level editable default for body cells)
- `structureControlsVisible` (default `true`)
- `addRowControlsVisible` (default `true`)
- `addColumnControlsVisible` (default `true`)
- `deleteContextMenuEnabled` (default `true`)
- `structureGutterWidth`, `structureGutterHeight`
- `defaultHeaderText`, `defaultCellText`
- `contextRowIndex`, `contextColumnIndex`

Signals:

- `cellInputEdited(rowIndex, columnIndex, text)`
- `cellInputSubmitted(rowIndex, columnIndex, text)`
- `cellInputRejected(rowIndex, columnIndex, text, valueType)`
- `cellsMerged(rowIndex, columnIndex, rowSpan, columnSpan)`
- `cellSplit(rowIndex, columnIndex)`
- `rowInserted(rowIndex)`
- `rowDeleted(rowIndex)`
- `columnInserted(columnIndex)`
- `columnDeleted(columnIndex)`
- `columnResized(columnIndex, width)`
- `rowResized(rowIndex, height)`

Helper methods:

- `rowAt(index)`
- `cellAt(rowIndex, columnIndex)`
- `columnCountForRow(rowEntry)`
- `autoCellWidth(rowEntry)`
- `columnWidth(columnIndex)`, `columnX(columnIndex)`, `columnSpanWidth(columnIndex, columnSpan)`
- `rowHeightAt(rowIndex)`, `rowY(rowIndex)`, `rowSpanHeight(rowIndex, rowSpan)`, `totalBodyHeight()`
- `rowCellWidth(rowEntry)`
- `rowCellSpacing(rowEntry)`
- `cellX(rowEntry, columnIndex)`
- `cellSpanWidth(rowEntry, columnSpan)`
- `rowInputable(rowEntry)`
- `cellInputable(rowIndex, columnIndex)`
- `headerAt(index)`
- `headerCellType(columnIndex)`, `columnType(columnIndex)`
- `normalizeHeaderCellType(value)`, `inferredCellType(value)`
- `cellRawValue(rowIndex, columnIndex)`
- `typedDefaultValue(valueType)`
- `coerceCellValue(value, valueType)`
- `validateCellInput(rowIndex, columnIndex, value)`
- `cellValueAccepted(rowIndex, columnIndex, value)`
- `cellText(rowIndex, columnIndex)`
- `setCellValue(rowIndex, columnIndex, value)`
- `cellRowSpan(rowIndex, columnIndex)`
- `cellColumnSpan(rowIndex, columnIndex)`
- `isCoveredCell(rowIndex, columnIndex)`
- `mergeAnchorForCell(rowIndex, columnIndex)`
- `canMergeCells(rowIndex, columnIndex, rowSpan, columnSpan)`
- `mergeCells(rowIndex, columnIndex, rowSpan, columnSpan)`
- `splitCell(rowIndex, columnIndex)`
- `canMutateStructure()`
- `insertRow(rowIndex)`, `appendRow()`, `deleteRow(rowIndex)`, `removeRow(rowIndex)`
- `insertColumn(columnIndex)`, `appendColumn()`, `deleteColumn(columnIndex)`, `removeColumn(columnIndex)`
- `canInsertRow(rowIndex)`, `canDeleteRow(rowIndex)`
- `canInsertColumn(columnIndex)`, `canDeleteColumn(columnIndex)`
- `buildContextMenuItems(rowIndex, columnIndex)`
- `openContextMenuForCell(rowIndex, columnIndex, menu, item, xPos, yPos)`
- `setColumnWidth(columnIndex, width)`, `setRowHeight(rowIndex, height)`
- `beginColumnResize(columnIndex, pointerX)`, `updateColumnResize(pointerX)`, `endColumnResize()`
- `beginRowResize(rowIndex, pointerY)`, `updateRowResize(pointerY)`, `endRowResize()`
- `undo()`, `redo()`, `clearUndoStack()`

## Usage

```qml
import LVRS 1.0 as LV

LV.Table {
    headerCellItems: [
        { label: "Name", type: "string" },
        { label: "State", type: "bool" },
        { label: "Score", type: "float" }
    ]
    rows: [
        [{ text: "Renderer" }, { value: true }, { value: 0.98 }],
        [{ text: "Metrics" }, { value: false }, { value: 0.72 }]
    ]
    columnWidths: [160, 80, 120]
    rowHeights: [28, 24]
}
```

## How It Works

- `TableModel` owns header resolution, body row lookup, column type inference, value coercion, merge/split metadata, and row/column structure mutations.
- `TableModel` also owns table geometry, row/column resize state, cell context-menu descriptors, and context action dispatch.
- `Table.qml` forwards model methods to `TableModel`, renders backend-provided cell descriptors, and emits public QML signals after backend mutations succeed.
- `TableModel` records array-backed cell edits, merge/split operations, row/column insert/delete operations, and row/column resize operations in a C++ `ModelUndoStack`.
- When `rows` is a C++ `QAbstractItemModel`, `TableModel` keeps that model object as the source and renders from `rowCount`, `columnCount`, `data`, and horizontal `headerData`.
- Model-backed `setCellValue(...)` calls the source model's `setData(index, coercedValue, Qt::EditRole)`, then falls back to named `value`, `edit`, and `text` roles when present. QML does not replace `rows` with a snapshot after the edit.
- Header entries define body column types. Objects may declare `type`, `valueType`, `cellType`, or `dataType`; primitive entries infer type from the primitive itself.
- `TableModel.visibleCells()` already includes `x`, `y`, `width`, and `height`; covered cells are skipped and anchor cells receive merged width/height.
- Each visible body cell instantiates `cellDelegate` and injects one `modelData` object. The descriptor includes `index`, `rowIndex`, `columnIndex`, `cellData`, `text`, `valueType`, `inputable`, `rowSpan`, `columnSpan`, `x`, `y`, `width`, and `height`.
- Custom body-cell delegates should declare `property var modelData`; their root item is sized to the backend-computed cell rectangle.
- `headerCellDelegate` uses the `TableHeader.cellDelegate` contract, so header and body rendering can be customized independently.
- Editable behavior propagates `Table.inputable -> row inputable -> cell inputable -> TableCellItem.inputable` through `TableModel`.
- Each body `TableCellItem` receives an injected validator from `Table`, which delegates to `TableModel` so inline edits are constrained by the header column type.
- Header and row counts are resolved for JS arrays, model-like objects, and C++ item models; structure editing stays limited to mutable array/list inputs.
- Backend geometry computes width from `columnWidths`, then `cellWidth`, then auto-fit column width.
- Backend geometry computes height from `rowHeights`, then `rowHeight`.
- Structure controls reserve a right gutter for row add buttons and a bottom gutter for column add buttons when `rows` is mutable.
- Resize handles sit on column right borders and row bottom borders when `resizeHandlesVisible` is enabled.
- Table container clips content and enforces internal divider contract.

## Typed Columns

Supported column types are:

- `string`
- `int`
- `float`
- `bool`

Object headers may declare any of these equivalent type keys:

```qml
headerCellItems: [
    { label: "Name", type: "string" },
    { label: "Count", valueType: "int" },
    { label: "Ratio", cellType: "float" },
    { label: "Enabled", dataType: "bool" }
]
```

Primitive headers infer type directly:

```qml
headerCellItems: ["Name", 1, 1.5, true]
```

`coerceCellValue(value, valueType)` returns `{ accepted, type, value, text }`. `setCellValue(rowIndex, columnIndex, value)` applies the same type check through `TableModel` and syncs accepted values back to array rows or writes them to the C++ item model through `setData`.

```qml
LV.Table {
    id: table
    headerCellItems: [
        { label: "Name", type: "string" },
        { label: "Count", type: "int" },
        { label: "Visible", type: "bool" }
    ]
    rows: [[{ value: "Renderer" }, { value: 3 }, { value: true }]]

    Component.onCompleted: {
        table.setCellValue(0, 1, "4")      // accepted, stores number 4
        table.setCellValue(0, 1, "4.2")    // rejected for int
        table.setCellValue(0, 2, "false")  // accepted, stores boolean false
    }
}
```

## Delegate Example

```qml
Component {
    id: bodyCell

    LV.TableCellItem {
        property var modelData: ({})
        itemData: modelData.cellData
        text: modelData.text
        valueType: modelData.valueType
        inputable: modelData.inputable === true
    }
}

LV.Table {
    headerCellItems: [{ label: "Name" }, { label: "Count", type: "int" }]
    rows: [[{ value: "Renderer" }, { value: 3 }]]
    cellDelegate: bodyCell
}
```

## Merge And Split

Static data may declare:

- `rowSpan`: number of rows the anchor cell covers.
- `columnSpan`: number of columns the anchor cell covers.
- `colSpan`: compatibility alias for `columnSpan`.

Runtime methods:

```qml
LV.Table {
    id: table
    rows: [
        [{ text: "A1" }, { text: "B1" }, { text: "C1" }],
        [{ text: "A2" }, { text: "B2" }, { text: "C2" }]
    ]

    Component.onCompleted: {
        table.mergeCells(0, 0, 2, 2)
        table.splitCell(0, 0)
    }
}
```

`mergeCells(...)` delegates to `TableModel`, which normalizes covered cells to objects and marks them as internal merge members. `splitCell(...)` accepts either the anchor cell or any covered cell and restores the covered cells as visible cells.

`mergeCells(...)` returns `false` and warns if `rows` is not a JavaScript array of row arrays or if the requested rectangle is out of bounds. Model-like read-only inputs can still render static `rowSpan`/`columnSpan`, but runtime mutation requires array rows.

## Structure Editing

When `structureControlsVisible` is enabled and `rows` is a JavaScript array of row arrays:

- a `+` button appears at the right end of each body row; clicking it inserts a row after that row,
- a `+` button appears at the bottom end of each column; clicking it inserts a column after that column,
- right-clicking a body cell opens that cell's own context menu with row and column deletion.

Programmatic calls:

```qml
LV.Table {
    id: table
    rows: [
        [{ text: "A1" }, { text: "B1" }],
        [{ text: "A2" }, { text: "B2" }]
    ]

    Component.onCompleted: {
        table.appendRow()
        table.appendColumn()
        table.deleteRow(0)
        table.deleteColumn(0)
    }
}
```

Runtime structure edits require mutable array rows. Before row/column insertion or deletion, existing span metadata is normalized so stale merged-cell anchors cannot point at the wrong row or column after the structure changes. Column deletion is rejected when the table has only one column left.

The delete context menu is not a table-level popup. Each rendered body cell owns its own `ContextMenu`, and `openContextMenuForCell(...)` rejects non-cell coordinates such as header-only or row-only context requests.

## C++ Model Rows

`rows` can point directly at a C++ `QAbstractItemModel` exposed to QML:

```qml
LV.Table {
    inputable: true
    rows: nativeTableModel
    headerCellItems: [
        { label: "Name", type: "string" },
        { label: "Count", type: "int" }
    ]
}
```

The minimum C++ editing contract is the standard Qt model contract:

```cpp
QVariant data(const QModelIndex &index, int role) const override;
bool setData(const QModelIndex &index, const QVariant &value, int role) override;
Qt::ItemFlags flags(const QModelIndex &index) const override;
```

Return display/edit values from `Qt::DisplayRole` and `Qt::EditRole`, mark editable cells with `Qt::ItemIsEditable`, and accept updates in `setData(index, value, Qt::EditRole)`. Header labels come from horizontal `headerData(..., Qt::DisplayRole)` unless `headerCellItems` or custom `headerColumns` are supplied.

## Undo And Redo

`Table` exposes the C++ history stack through:

- `canUndo`, `canRedo`
- `undoDepth`, `redoDepth`
- `undo()`, `redo()`, `clearUndoStack()`

Recorded operations:

- `setCellValue(...)`
- `mergeCells(...)`
- `splitCell(...)`
- `insertRow(...)`, `deleteRow(...)`
- `insertColumn(...)`, `deleteColumn(...)`
- `setColumnWidth(...)`, `setRowHeight(...)`
- resize drag updates through `begin*/update*/end*Resize(...)`

```qml
LV.Table {
    id: table
    rows: [[{ value: "Renderer" }, { value: 3 }]]

    Component.onCompleted: {
        table.setCellValue(0, 1, "4")
        table.undo()
        table.redo()
    }
}
```

## Resize Editing

Users can drag column right borders to update `columnWidths[columnIndex]` and row bottom borders to update `rowHeights[rowIndex]`. The drag state and size mutation live in `TableModel`; QML handles only forward pointer coordinates from the visible handles.

Programmatic calls use the same sizing path:

```qml
LV.Table {
    id: table
    rows: [
        [{ text: "A1" }, { text: "B1" }],
        [{ text: "A2" }, { text: "B2" }]
    ]

    Component.onCompleted: {
        table.setColumnWidth(0, 180)
        table.setRowHeight(1, 36)
    }
}
```

Widths and heights are clamped by `minColumnWidth` and `minRowHeight`. `beginColumnResize/updateColumnResize/endColumnResize` and the row equivalents are exposed so tests or custom handles can reuse the built-in drag state machine.

## Advanced Example: Object Rows

```qml
import LVRS 1.0 as LV

LV.Table {
    headerCellItems: [
        { label: "Service" },
        { label: "State" },
        { label: "Owner" }
    ]
    rows: [
        [{ text: "Renderer", rowSpan: 2 }, { text: "Active" }, { text: "Core" }],
        [{ text: "Input" }, { text: "Idle" }, { text: "UX" }]
    ]
}
```

Cell text fallback supports `label/text/title` object keys.
