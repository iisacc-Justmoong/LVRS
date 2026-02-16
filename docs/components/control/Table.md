# Table

Location: `qml/components/control/display/Table.qml`

`Table` is a compact display surface that composes `TableHeader` + repeated `TableRow`.

## Core API

- `headerColumns` (`var`): header labels
- `rows` (`var`): row/cell data
- `rowHeight` (`int`, default `24`)
- `cellWidth` (`int`, default `0`, auto-fit)
- `backgroundColor` (`color`, default `#282828`)
- `borderColor` / `borderWidth`

The default size is aligned with the Figma table node: `405x121`.

## Usage

```qml
LV.Table {
    width: 405
    headerColumns: ["Name", "State", "Owner"]
    rows: [
        ["Renderer", "Active", "Core"],
        ["Input", "Idle", "UX"],
        ["Pipeline", "Active", "Render"],
        ["Metrics", "Paused", "Ops"]
    ]
}
```
