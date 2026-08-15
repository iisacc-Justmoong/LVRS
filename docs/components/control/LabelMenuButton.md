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
- injected method API inherited from `AbstractButton`: `method`, `methods`, `invokeMethods(...)`

Indicator:

- `indicatorSize` (readonly, `Theme.iconSm`: desktop `18 x 18`, mobile `23 x 23`)
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
- `verticalPadding: Theme.scaleMetric(1)`
- `spacing: Theme.gap2`

## Usage

```qml
import LVRS 1.0 as LV

LV.LabelMenuButton {
    text: "Options"
    tone: LV.AbstractButton.Borderless
    method: function(eventData) {
        menu.open()
    }
}
```
