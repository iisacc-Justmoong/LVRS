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

Signals:

- `cellInputEdited(rowIndex, columnIndex, text)`
- `cellInputSubmitted(rowIndex, columnIndex, text)`
- `cellsMerged(rowIndex, columnIndex, rowSpan, columnSpan)`
- `cellSplit(rowIndex, columnIndex)`

Helper methods:

- `rowAt(index)`
- `cellAt(rowIndex, columnIndex)`
- `columnCountForRow(rowEntry)`
- `autoCellWidth(rowEntry)`
- `rowCellWidth(rowEntry)`
- `rowCellSpacing(rowEntry)`
- `cellX(rowEntry, columnIndex)`
- `cellSpanWidth(rowEntry, columnSpan)`
- `rowInputable(rowEntry)`
- `cellInputable(rowIndex, columnIndex)`
- `cellText(rowIndex, columnIndex)`
- `cellRowSpan(rowIndex, columnIndex)`
- `cellColumnSpan(rowIndex, columnIndex)`
- `isCoveredCell(rowIndex, columnIndex)`
- `mergeAnchorForCell(rowIndex, columnIndex)`
- `canMergeCells(rowIndex, columnIndex, rowSpan, columnSpan)`
- `mergeCells(rowIndex, columnIndex, rowSpan, columnSpan)`
- `splitCell(rowIndex, columnIndex)`

## Usage

```qml
import LVRS 1.0 as LV

LV.Table {
    headerCellItems: [
        { label: "Name" },
        { label: "State" },
        { label: "Owner" }
    ]
    rows: [
        [{ text: "Renderer", columnSpan: 2 }, { text: "Active" }, { text: "Core" }],
        [{ text: "Metrics" }, { text: "Paused" }, { text: "Ops" }]
    ]
}
```

## How It Works

- Header source resolves from `headerCellItems` first, then `headerColumns`.
- Row entries are flattened into `visibleCellItems`; covered cells are skipped and anchor cells receive merged width/height.
- Editable behavior propagates `Table.inputable -> row inputable -> cell inputable -> TableCellItem.inputable`.
- Header and row counts are resolved for both JS arrays and model-like objects.
- Cell delegates compute width either from fixed `cellWidth` or the per-row auto-fit formula.
- Table container clips content and enforces internal divider contract.

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
