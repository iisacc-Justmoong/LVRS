# ToggleSwitch

Location: `qml/components/control/check/ToggleSwitch.qml`

`ToggleSwitch` is an LVRS-styled switch built on `QtQuick.Controls.Switch` and aligned to the Figma two-state toggle set.

## Purpose

- Keep switch geometry and animation deterministic across platforms.
- Expose explicit track, knob, shadow, palette, and shape control.

## Core API

Shape and metrics:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `trackWidth`, `trackHeight`, `trackPadding`
- `knobSize`
- `trackCornerRadius`, `knobCornerRadius`
- `transitionDuration`

Figma shadow:

- `trackShadowEnabled`
- `trackShadowColor`, `trackShadowOpacity`
- `trackShadowBlur`
- `trackShadowHorizontalOffset`, `trackShadowVerticalOffset`

Palette:

- `onColor`, `offColor`
- `onColorHover`, `onColorPressed`
- `offColorHover`, `offColorPressed`
- `disabledTrackColor`
- `trackShadowColor`
- `knobFillColor`
- resolved: `resolvedTrackColor`

State (inherited):

- `checked`, `enabled`, `text`

Figma compatibility:

- `state` <-> `checked`

## Behavior Contract

- The Figma component set (`111:379`) contains On and Off variants, each `38 x 22` with `2px` inner padding, an `18 x 18` knob, and a `20px` authored radius.
- On resolves to `Theme.accent`; Off resolves to `Theme.panelBackground12`; the knob resolves to `Theme.titleHeaderColor`.
- The track uses the Figma `0px 4px 4px` shadow with `Theme.shadowStrong` (`25%` black). `QtQuick.Effects.MultiEffect` supplies the blur and automatically padded overflow.
- Knob x-position animates between `2` (Off) and `18` (On) on desktop.
- Knob fill is an antialiased scene-graph `Rectangle`, so its circle remains resolution-independent without a raster canvas. The previous read-only `knobSupersampleScale`, `knobHiDpiScale`, and `knobRasterScale` values remain available for source compatibility.
- Track color resolves from `checked + hovered + down + enabled`.
- Desktop and mobile share the `38 x 22` track, `2` padding, `18 x 18` knob, radii `20` and `9`, On/Off x positions `18`/`2`, and shadow blur/vertical offset `4`.
- Optional label spacing scales from `8px` to `16px`, while the Body font and line height remain fixed at `13px/13px`.

## Usage

```qml
import LVRS 1.0 as LV

LV.ToggleSwitch {
    checked: true
}
```
