# LabelMenuButton

Location: `qml/components/control/buttons/LabelMenuButton.qml`

`LabelMenuButton` is a label preset of [DropdownButton](DropdownButton.md).

## Purpose

- Provide compact text-first menu trigger.
- Keep indicator behavior consistent with tone and enabled state.

## Core API

Text and tone:

- `text` (inherited)
- `tone` (inherited, default: `Primary`, matching `Kind=accent`)
- injected method API inherited from `AbstractButton`: `method`, `methods`, `invokeMethods(...)`

Indicator:

- `indicatorSize` (readonly, `Theme.iconSm`: desktop `18 x 18`, mobile `36 x 36`)
- `resolvedIndicatorName` from tone/enabled state
- rendered via `Theme.iconPath(...)`
- supersampling-aware icon source size (`indicatorSourceSize`)

Indicator icon mapping:

- default: `generalchevronDown`
- borderless: `generalchevronDownBorderless`
- primary/destructive: `generalchevronDownAccent`
- disabled: `generalchevronDownDisabled`

Layout:

- fixed `figmaButtonHeight`: `22` desktop, `44` mobile
- left inset: `horizontalPadding: Theme.gap8` (`8` desktop, `16` mobile)
- independent right inset: `rightPadding: Theme.gap2` (`2` desktop, `4` mobile)
- `cornerRadius: Theme.radiusMd` (`8` desktop, `16` mobile)
- `verticalPadding: Theme.gap2` (`2` desktop, `4` mobile)
- label-chevron gap: `spacing: Theme.gapNone` (`0`)

## Figma Visual Contract

- Source: `700:337`, `Type=LabelMenuButton`.
- With text `Open`, the desktop frame is `60 x 22`: text `x=8, y=4.5, 32 x 13`, chevron `x=40, y=2, 18 x 18`.
- Body typography remains fixed at `13px Medium / 13px` on desktop and mobile.
- The corresponding mobile token composition is `88 x 44`.
- Existing tone-specific `generalchevronDown*.svg` assets match the Figma vectors and are reused.
- When an explicit width is narrower than the implicit frame, the label elides while the chevron remains inside the right inset.

## Usage

```qml
import LVRS 1.0 as LV

LV.LabelMenuButton {
    text: "Options"
    method: function(eventData) {
        menu.open()
    }
}
```
