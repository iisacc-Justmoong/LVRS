# IconMenuButton

Location: `qml/components/control/buttons/IconMenuButton.qml`

`IconMenuButton` is an icon-first menu trigger with a trailing chevron indicator.

## Purpose

- Provide compact menu trigger for icon-centric toolbars.
- Keep icon fallback and indicator rendering deterministic.

## Core API

Main icon:

- `iconSource` (alias of `url`)
- `iconName`
- `iconGlyph` (text glyph fallback)
- `iconSize`

Indicator:

- `indicatorSize` (readonly, `Theme.iconSm`)
- tone/enable-aware indicator selection (`resolvedIndicatorName`)
- rendered from `Theme.iconPath(...)`
- indicator source uses supersampling-aware `sourceSize`

Indicator icon mapping:

- default: `generalchevronDown`
- borderless: `generalchevronDownBorderless`
- primary/destructive: `generalchevronDownAccent`
- disabled: `generalchevronDownDisabled`

Layout:

- default tone fallback: `Borderless`
- main and indicator icon frames: `Theme.iconSm` (`18 x 18` on desktop, `23 x 23` on mobile)
- fixed `figmaButtonHeight` (`Theme.gap20`: desktop `20`, mobile `25`)
- `horizontalPadding: Theme.scaleMetric(1)`
- `verticalPadding: Theme.scaleMetric(1)`
- `spacing: Theme.gap4`

Injected methods:

- inherited from `AbstractButton`: `method`, `methods`, `hasInjectedMethods`, `invokeMethods(...)`

## Icon Resolution Order

1. `iconSource` (`url`)
2. `iconName`
3. grouped `icon.name` (if provided by parent style object)
4. fallback icon (`projectStructure`)

## Usage

```qml
import LVRS 1.0 as LV

LV.IconMenuButton {
    tone: LV.AbstractButton.Borderless
    iconName: "projectStructure"
    method: function(eventData) {
        menu.open()
    }
}
```
