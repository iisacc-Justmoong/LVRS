# ToggleSwitch

Location: `qml/components/control/check/ToggleSwitch.qml`

`ToggleSwitch` is an LVRS-styled switch built on `QtQuick.Controls.Switch`.

## Purpose

- Keep switch geometry and animation deterministic across platforms.
- Expose explicit track/knob palette and shape control.

## Core API

Shape and metrics:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `trackWidth`, `trackHeight`, `trackPadding`
- `knobSize`
- `trackCornerRadius`
- `transitionDuration`

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

## Behavior Contract

- Knob x-position animates between `knobXOff` and `knobXOn`.
- Knob fill is drawn by `Canvas` and repainted on `knobFillColor` change.
- Track color resolves from `checked + hovered + down + enabled`.

## Usage

```qml
import LVRS 1.0 as LV

LV.ToggleSwitch {
    checked: true
}
```
