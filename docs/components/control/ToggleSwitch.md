# ToggleSwitch

Location: `qml/components/control/check/ToggleSwitch.qml`

`ToggleSwitch` is a compact on/off control implemented on `QtQuick.Controls.Switch` with LVRS custom visuals.

## Purpose

- Provide deterministic switch geometry/animation across platforms.
- Support palette and metric customization while keeping behavior simple.

## API

Metrics:

- `trackWidth`, `trackHeight`, `trackPadding`
- `knobSize`
- `transitionDuration`

Colors:

- `onColor`, `offColor`
- `disabledTrackColor`
- `trackShadowColor`
- `knobFillColor`

Figma token defaults:

- `onColor -> Theme.accent`
- `offColor -> Theme.panelBackground12`
- `knobFillColor -> Theme.textPrimary`

State:

- `checked`, `enabled`, `text` (inherited)

## Usage

```qml
import LVRS 1.0 as LV

LV.ToggleSwitch {
    checked: true
    onToggled: console.log("state", checked)
}
```

## How It Works

- Knob x-position is animated between computed off/on coordinates.
- Knob is rendered with `Canvas` for stable circular fill rendering.
- Implicit width includes text label region only when text is non-empty.

## Advanced Example: Low-Contrast Disabled Palette

```qml
import LVRS 1.0 as LV

LV.ToggleSwitch {
    enabled: false
    checked: true
    disabledTrackColor: "#4B4B4B"
}
```

## Note

Switch animation duration should stay short for responsiveness; avoid long transitions in frequently toggled controls.

## FAQ

Q. Is `ToggleSwitch` suitable for irreversible destructive actions?  
A. No. Use explicit confirmation controls for destructive flows.
