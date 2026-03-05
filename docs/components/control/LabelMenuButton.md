# LabelMenuButton

Location: `qml/components/control/buttons/LabelMenuButton.qml`

`LabelMenuButton` is a text + chevron menu trigger based on `AbstractButton`.

## Purpose

- Provide compact text-first menu trigger.
- Keep indicator behavior consistent with tone and enabled state.

## Core API

Text and tone:

- `text` (inherited)
- `tone` (inherited, default fallback: `Borderless`)

Indicator:

- `resolvedIndicatorName` from tone/enabled state
- rendered via `Theme.iconPath(...)`
- supersampling-aware icon source size (`indicatorSourceSize`)

Indicator icon mapping:

- default: `generalchevronDown`
- borderless: `generalchevronDownBorderless`
- primary/destructive: `generalchevronDownAccent`
- disabled: `generalchevronDownDisabled`

Layout:

- fixed `figmaButtonHeight` (`Theme.gap20`)
- `horizontalPadding: Theme.gap8`
- `verticalPadding: Theme.gap2`
- `spacing: Theme.gap2`

## Usage

```qml
import LVRS 1.0 as LV

LV.LabelMenuButton {
    text: "Options"
    tone: LV.AbstractButton.Borderless
}
```
