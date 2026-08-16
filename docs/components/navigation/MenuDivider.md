# MenuDivider

Location: `qml/components/navigation/MenuDivider.qml`

`MenuDivider` is a one-axis separator line used between menu groups.

## Purpose

- Provide a minimal divider that can be horizontal or vertical.
- Keep separator rendering deterministic with a single axis input.

## API

- `axis`: `"horizontal"` or `"vertical"` (default: `"horizontal"`)
- `dividerColor` (default: `Theme.contextMenuDivider` = `Theme.panelBackground08`)
- `thickness` (default: `1`)
- `crossPadding` (default: `1`)
- `linePadding` (default: `0`)
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
- Horizontal mode resolves to `220 x 3` and draws a centered `220 x 1` rule at `x=0, y=1`.
- Vertical mode applies the same padding contract with the axes exchanged.
- `crossPadding` centers the rule on the cross axis. `linePadding` remains customizable but defaults to `0` because the current Figma divider spans its full assigned width.
- Figma source: node `110:853` (`MenuDivider`). A divider stretched by `ContextMenu` uses the menu content width (`145px`).

## Practical Notes

- Use only `axis` to choose orientation as the primary control.
- Keep `thickness: 1`, `crossPadding: 1`, and `linePadding: 0` for Figma parity unless a different separator is explicitly required.
