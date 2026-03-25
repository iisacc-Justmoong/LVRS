# MenuDivider

Location: `qml/components/navigation/MenuDivider.qml`

`MenuDivider` is a one-axis separator line used between menu groups.

## Purpose

- Provide a minimal divider that can be horizontal or vertical.
- Keep separator rendering deterministic with a single axis input.

## API

- `axis`: `"horizontal"` or `"vertical"` (default: `"horizontal"`)
- `dividerColor` (default: `Theme.contextMenuDivider` = `Theme.disabledColor`)
- `thickness` (default: `0.2`)
- `crossPadding` (default: `1`)
- `lineLength` (default: `220`)

Computed:

- `verticalAxis` (readonly)

## Usage

```qml
import LVRS 1.0 as LV

LV.MenuDivider {
    axis: "horizontal"
}
```

```qml
import LVRS 1.0 as LV

LV.MenuDivider {
    axis: "vertical"
    lineLength: 80
}
```

## How It Works

- `axis` is normalized case-insensitively.
- Horizontal mode expands width and draws a full-width hairline rule.
- Vertical mode expands height and draws a full-height hairline rule.
- `crossPadding` applies on the cross axis so the line remains visually centered; the default horizontal divider therefore resolves to a `2.2px`-tall row with a centered `0.2px` rule before device scaling.

## Practical Notes

- Use only `axis` to choose orientation as the primary control.
- Keep `thickness` at `0.2` for the current Figma parity unless a heavier separator is explicitly required.
