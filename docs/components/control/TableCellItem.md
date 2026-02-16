# TableCellItem

Location: `qml/components/control/display/TableCellItem.qml`

`TableCellItem` is the smallest table primitive: left divider + single text entry.

## Core API

- `text` (`string`)
- `cellHeight` (`int`, default `24`)
- `contentSpacing` (`int`, default `Theme.gap8`)
- `dividerColor` (`color`, default `Theme.surface`)
- `textColor` (`color`, default `Theme.bodyColor`)
- `showDivider` (`bool`, default `true`)

Typography follows `LV.Label` `body` style (`12 / Medium`).

## Usage

```qml
LV.TableCellItem {
    text: "Text"
}
```
