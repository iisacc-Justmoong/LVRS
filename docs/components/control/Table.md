# Table

Location: `qml/components/control/display/Table.qml`

`Table` composes `TableHeader` and positioned `TableCellItem` delegates for compact data display with optional cell spanning.

## Purpose

- Provide fixed-height, dense tabular display.
- Accept array/list-model style inputs for headers and rows.
- Support cell merge/split behavior through span metadata and table-level mutation methods.

## API

Data:

- `headerCellItems` (preferred)
- `headerColumns`
- `rows`

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
- `visibleCellItems` (readonly flattened render model)

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
- `buildContextMenuItems()`
- `openContextMenuForCell(rowIndex, columnIndex, item, xPos, yPos)`
- `setColumnWidth(columnIndex, width)`, `setRowHeight(rowIndex, height)`
- `beginColumnResize(columnIndex, pointerX)`, `updateColumnResize(pointerX)`, `endColumnResize()`
- `beginRowResize(rowIndex, pointerY)`, `updateRowResize(pointerY)`, `endRowResize()`

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

- Header source resolves from `headerCellItems` first, then `headerColumns`.
- Header entries define body column types. Objects may declare `type`, `valueType`, `cellType`, or `dataType`; primitive entries infer type from the primitive itself.
- Row entries are flattened into `visibleCellItems`; covered cells are skipped and anchor cells receive merged width/height.
- Editable behavior propagates `Table.inputable -> row inputable -> cell inputable -> TableCellItem.inputable`.
- Each body `TableCellItem` receives an injected validator from `Table`, so inline edits are constrained by the header column type.
- Header and row counts are resolved for both JS arrays and model-like objects.
- Cell delegates compute width from `columnWidths`, then `cellWidth`, then auto-fit column width.
- Cell delegates compute height from `rowHeights`, then `rowHeight`.
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

`coerceCellValue(value, valueType)` returns `{ accepted, type, value, text }`. `setCellValue(rowIndex, columnIndex, value)` applies the same type check and mutates array-backed `rows` only when the value is accepted.

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

`mergeCells(...)` mutates array-backed `rows` by normalizing covered cells to objects and marking them as internal merge members. `splitCell(...)` accepts either the anchor cell or any covered cell and restores the covered cells as visible cells.

`mergeCells(...)` returns `false` and warns if `rows` is not a JavaScript array of row arrays or if the requested rectangle is out of bounds. Model-like read-only inputs can still render static `rowSpan`/`columnSpan`, but runtime mutation requires array rows.

## Structure Editing

When `structureControlsVisible` is enabled and `rows` is a JavaScript array of row arrays:

- a `+` button appears at the right end of each body row; clicking it inserts a row after that row,
- a `+` button appears at the bottom end of each column; clicking it inserts a column after that column,
- right-clicking a body cell opens a context menu with row and column deletion,
- right-clicking a header column opens a context menu with column deletion.

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

## Resize Editing

Users can drag column right borders to update `columnWidths[columnIndex]` and row bottom borders to update `rowHeights[rowIndex]`.

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
