# PushButton

Location: `qml/components/control/buttons/PushButton.qml`

`PushButton` is the independent push-action family built on `AbstractButton`.
It implements [Figma PushButton 44:599](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=44-599),
measured on 2026-09-06. `LabelButton` and `IconButton` are its label and icon presets.

## API

- `iconMode: false` selects the label variant; `true` selects the icon variant.
- `text`, `tone`, `enabled`, `method`, `methods`, and `clicked()` retain the
  `AbstractButton` contract. `Primary` is the default tone.
- Icon inputs: `iconSource` / `url`, `iconName`, grouped `icon.name`, `iconGlyph`,
  and `iconSize`. The existing project-structure fallback and supersampling are reused.
- The icon preset also supports optional text beside its icon.
- Padding, `spacing`, and `cornerRadius` remain independently overridable.

## Geometry

| Variant | Desktop size | Padding L / T / R / B | Gap | Radius |
| --- | --- | --- | --- | --- |
| Label, text `Button` | 56 × 22 | 8 / 4.5 / 8 / 4.5 | 10 | 8 |
| Icon, accent | 22 × 22 | 2 / 2 / 2 / 2 | 0 | 8 |
| Icon, other tones | 22 × 22 | 2 / 2 / 2 / 2 | 7 | 8 |

Figma's gap is preserved even when the variant has only one child. The label uses
the existing fixed Body 13px Medium / 13px line box. The icon is 18 × 18.
Radius uses `Theme.radiusMd`; gaps use `Theme.gap10`, `Theme.gap7`, and `Theme.gapNone`.
On mobile, geometry tokens double while the Body line box remains 13px: label
72 × 44, icon 44 × 44, radius 16, and label vertical padding 15.5.
Default, accent, borderless, destructive, and disabled variants share their size.

## Usage

```qml
import LVRS 1.0 as LV

LV.PushButton {
    text: "Save"
    method: function(eventData) { documentController.save() }
}
LV.PushButton { iconMode: true; iconName: "add" }
```

`documentController` belongs to the consuming application. `LabelButton` and
`IconButton` remain usable without changing imports or method injection.

## Validation

`LVRSTests_import_api::button_family_components_contract` loads both new types,
checks both content modes and every existing tone on desktop and mobile, and
verifies dimensions, four padding values, radius, gap, chevron bounds, actual
click dispatch, and disabled-input blocking. `button_padding_matches_figma_spec`
checks the existing preset names against the same updated geometry.
Set `LVRS_BUTTON_CAPTURE_DIR` to a directory under `build/` to save the rendered
desktop and mobile matrices. Run `ctest --test-dir build --output-on-failure`
after `cmake --build build` with the build-tree LVRS library in the runtime path.
