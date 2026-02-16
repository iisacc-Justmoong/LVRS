# TableRow

Location: `qml/components/control/display/TableRow.qml`

`TableRow` arranges repeated `TableCellItem` delegates for one row.

## Core API

- `cells` (`var`): row entries
- `cellWidth` (`int`, default `234`)
- `cellHeight` (`int`, default `24`)
- `contentSpacing` (`int`, default `Theme.gap8`)
- `dividerColor` (`color`)
- `textColor` (`color`, default `Theme.bodyColor`)

Row spacing is computed from available row width to preserve Figma distribution behavior.

## Usage

```qml
LV.TableRow {
    width: 717
    cells: ["Text", "Text", "Text"]
}
```
