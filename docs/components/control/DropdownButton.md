# DropdownButton

Location: `qml/components/control/buttons/DropdownButton.qml`

`DropdownButton` is the independent menu-trigger family built on `AbstractButton`.
It implements [Figma DropdownButton 700:337](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=700-337),
measured on 2026-09-06. `LabelMenuButton` and `IconMenuButton` are its label and icon presets.

## API

- `iconMode: false` selects label + chevron; `true` selects icon + chevron.
- `text`, `tone`, `enabled`, `method`, `methods`, and `clicked()` retain the
  `AbstractButton` contract. `Primary` is the default tone.
- Icon inputs: `iconSource` / `url`, `iconName`, grouped `icon.name`, `iconGlyph`,
  and `iconSize`. The project-structure fallback and tone-specific chevrons are reused.
- `indicatorSize`, `resolvedIndicatorName`, and `renderedIndicatorSource` expose
  the existing chevron contract, including supersampling.
- The caller opens its menu through `method` or `onClicked`; the button does not
  own or create the application's menu model.

## Geometry

| Variant | Desktop size | Padding L / T / R / B | Gap | Radius |
| --- | --- | --- | --- | --- |
| Label, text `Open` | 60 × 22 | 8 / 2 / 2 / 2 | 0 | 8 |
| Icon | 40 × 22 | 4 / 2 / 2 / 2 | -2 | 8 |

The label occupies 32 × 13 at (8, 4.5); its 18 × 18 chevron starts at (40, 2).
The icon occupies 18 × 18 at (4, 2); its chevron starts at (20, 2).
`rightPadding` defaults to `Theme.gap2` independently of `horizontalPadding`.
The latter sets the left inset to `Theme.gap8` or `Theme.gap4`. The gap remains
overridable through `spacing`; radius uses `Theme.radiusMd`.

On mobile, geometry tokens double and Body typography remains 13px: label
88 × 44, icon 80 × 44, radius 16, and icon-chevron gap -4. With a narrower custom
label-button width, text elides to preserve the chevron and its right inset.

The current Figma set supplies accent, default, and borderless references. The
inherited destructive tone and disabled behavior remain available for existing
consumers; they use the same measured geometry.

## Usage

```qml
import LVRS 1.0 as LV

LV.DropdownButton {
    text: "Open"
    method: function(eventData) { menu.open() }
}
LV.DropdownButton { iconMode: true; iconName: "projectStructure" }
```

`menu` belongs to the consuming application. Existing `LabelMenuButton` and
`IconMenuButton` imports inherit the updated geometry automatically.
See [PushButton validation](PushButton.md#validation) for shared verification.
