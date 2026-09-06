# IconMenuButton

Location: `qml/components/control/buttons/IconMenuButton.qml`

`IconMenuButton` is the `iconMode: true` preset of [DropdownButton](DropdownButton.md), with a trailing chevron.

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
- fixed frame: `40 x 22` desktop, `80 x 44` mobile
- left inset: `Theme.gap4`; right, top, bottom: `Theme.gap2` (`4 / 2 / 2 / 2` desktop)
- `cornerRadius: Theme.radiusMd` (`8` desktop, `16` mobile)
- measured icon-indicator overlap: `spacing: -Theme.gap2` (`-2` desktop, `-4` mobile); the content layout consumes this public property so callers can override it without replacing the component

## Figma Visual Contract

- Source: `700:337`, `Type=IconMenuButton`.
- Desktop main icon bounds are `x=4, y=2, 18 x 18`; chevron bounds are `x=20, y=2, 18 x 18`.
- Existing `generalprojectStructure.svg` and tone-specific `generalchevronDown*.svg` assets match the exported Figma vectors and are reused.
- The current Figma set has accent, default, and borderless variants; inherited destructive/disabled states retain the same geometry.

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
