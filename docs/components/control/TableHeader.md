# TableHeader

Location: `qml/components/control/display/TableHeader.qml`

`TableHeader` renders a single header row with a bottom separator line.

## Core API

- `columns` (`var`): header entries
- `rowHeight` (`int`, default `24`)
- `cellHorizontalPadding` (`int`, default `Theme.gap8`)
- `textColor` (`color`, default `Theme.descriptionColor`)
- `separatorHeight` / `separatorColor`

Typography follows `LV.Label` `description` style (`12 / SemiBold`).

## Usage

```qml
LV.TableHeader {
    width: 717
    columns: ["Column", "Column", "Column"]
}
```
