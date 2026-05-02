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
- fixed `figmaButtonHeight` (`Theme.gap20`)
- `horizontalPadding: Theme.gap2`
- `verticalPadding: Theme.gap2`
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
