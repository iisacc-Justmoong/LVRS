# Table

Location: `qml/components/control/display/Table.qml`

`Table` composes `TableHeader` and repeated `TableRow` delegates for compact read-only data display.

## Purpose

- Provide fixed-height, dense tabular display.
- Accept array/list-model style inputs for headers and rows.

## API

Data:

- `headerCellItems` (preferred)
- `headerColumns`
- `rows`

Layout:

- `rowHeight`
- `cellWidth` (`0` means auto width)

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

Helper methods:

- `rowAt(index)`
- `columnCountForRow(rowEntry)`
- `autoCellWidth(rowEntry)`
- `rowInputable(rowEntry)`

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
        [{ text: "Renderer" }, { text: "Active" }, { text: "Core" }],
        [{ text: "Metrics" }, { text: "Paused" }, { text: "Ops" }]
    ]
}
```

## How It Works

- Header source resolves from `headerCellItems` first, then `headerColumns`.
- Row entries are forwarded to `TableRow.cellItems` and each cell to `TableCellItem.itemData`.
- Editable behavior propagates `Table.inputable -> TableRow.inputable -> TableCellItem.inputable` unless per-row/per-cell override is provided.
- Header and row counts are resolved for both JS arrays and model-like objects.
- Row delegates compute width either from fixed `cellWidth` or auto-fit formula.
- Table container clips content and enforces internal divider contract.

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
        [{ text: "Renderer" }, { text: "Active" }, { text: "Core" }],
        [{ text: "Input" }, { text: "Idle" }, { text: "UX" }]
    ]
}
```

Cell text fallback supports `label/text/title` object keys.
