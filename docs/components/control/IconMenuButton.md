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

- default tone: `Primary` (`Kind=accent`)
- main and indicator icon frames: `Theme.iconSm` (`18 x 18` on desktop, `36 x 36` on mobile)
- fixed frame: `38 x 22` desktop, `76 x 44` mobile
- horizontal/vertical padding: `Theme.gap2` (`2` desktop, `4` mobile)
- measured icon-indicator overlap: `spacing: -Theme.gap2` (`-2` desktop, `-4` mobile); the content layout consumes this public property so callers can override it without replacing the component

## Figma Visual Contract

- Source: `44:599`, `Type=IconMenuButton`.
- Desktop main icon bounds are `x=2, y=2, 18 x 18`; chevron bounds are `x=18, y=2, 18 x 18`.
- Existing `generalprojectStructure.svg` and tone-specific `generalchevronDown*.svg` assets match the exported Figma vectors and are reused.
- All five tones preserve the same geometry.

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
    iconName: "projectStructure"
    method: function(eventData) {
        menu.open()
    }
}
```
