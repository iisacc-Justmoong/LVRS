# MenuDivider

Location: `/Users/ymy/Developer/LVRS/qml/components/navigation/MenuDivider.qml`

`MenuDivider` is a one-axis separator line used between menu groups.

## Purpose

- Provide a minimal divider that can be horizontal or vertical.
- Keep separator rendering deterministic with a single axis input.

## API

- `axis`: `"horizontal"` or `"vertical"` (default: `"horizontal"`)
- `dividerColor` (default: `Theme.contextMenuDivider` = `Theme.panelBackground08`)
- `thickness` (default: `1`)
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
- Horizontal mode expands width and draws a full-width 1px line.
- Vertical mode expands height and draws a full-height 1px line.
- `crossPadding` applies on the cross axis so the line remains visually centered.

## Practical Notes

- Use only `axis` to choose orientation as the primary control.
- Keep `thickness` at `1` for Figma parity unless a heavier separator is explicitly required.
