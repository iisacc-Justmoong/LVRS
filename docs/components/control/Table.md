# Table

Location: `qml/components/control/display/Table.qml`

`Table` composes `TableHeader` and repeated `TableRow` delegates for compact read-only data display.

## Purpose

- Provide fixed-height, dense tabular display.
- Accept array/list-model style inputs for headers and rows.

## API

Data:

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
- `dividerColor`

Helper methods:

- `rowAt(index)`
- `columnCountForRow(rowEntry)`
- `autoCellWidth(rowEntry)`

## Usage

```qml
import LVRS 1.0 as LV

LV.Table {
    headerColumns: ["Name", "State", "Owner"]
    rows: [
        ["Renderer", "Active", "Core"],
        ["Metrics", "Paused", "Ops"]
    ]
}
```

## How It Works

- Header and row counts are resolved for both JS arrays and model-like objects.
- Row delegates compute width either from fixed `cellWidth` or auto-fit formula.
- Table container clips content and enforces internal divider contract.

## Advanced Example: Object Rows

```qml
import LVRS 1.0 as LV

LV.Table {
    headerColumns: ["Service", "State", "Owner"]
    rows: [
        [{ text: "Renderer" }, { text: "Active" }, { text: "Core" }],
        [{ text: "Input" }, { text: "Idle" }, { text: "UX" }]
    ]
}
```

Cell text fallback supports `label/text/title` object keys.
