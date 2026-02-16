# TableCellItem

Location: `qml/components/control/display/TableCellItem.qml`

`TableCellItem` is the smallest table primitive: divider line + single text body.

## Purpose

- Render one cell with optional leading divider.
- Keep clipping and text ellipsis behavior consistent.

## API

- `text`
- `cellHeight`
- `contentSpacing`
- `dividerColor`
- `textColor`
- `showDivider`
- `clipContent`

## Usage

```qml
import LVRS 1.0 as LV

LV.TableCellItem {
    text: "Text"
}
```

## How It Works

- Divider visibility is explicit (`showDivider`).
- Text anchor margin is conditionally offset only when divider exists.
- Content clipping can be disabled for custom overflow behavior.

## Practical Tip

Disable `clipContent` only when cell overflow is intentionally part of the design (for example, animated marquee or hover expansion).

## Extended Example: Dividerless Cell

```qml
import LVRS 1.0 as LV

LV.TableCellItem {
    text: "Primary Value"
    showDivider: false
    clipContent: true
}
```

## FAQ

Q. Why is left margin still visible when divider is hidden?  
A. Margin is conditionally removed when `showDivider` is false. If spacing remains, inspect parent layout spacing.

## Validation Checklist

- divider visibility matches design requirement,
- text elision works under narrow widths,
- clipping behavior matches overflow policy.
