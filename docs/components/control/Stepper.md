# Stepper

Location: `qml/components/control/buttons/Stepper.qml`

`Stepper` is a compact standalone directional control that follows the Figma stepper contract (`16x16`, radius `4`) with `Up`, `Down`, and `UpDown` arrow modes.

## Purpose

- Provide a minimal increment/decrement trigger shape for dense control rows.
- Keep interaction states deterministic across `Primary` and `Borderless` tones.

## Core API

- `tone` (default: `AbstractButton.Primary`)
- `arrow` (default: `Stepper.UpDown`)
  - `Stepper.UpDown`
  - `Stepper.Up`
  - `Stepper.Down`

Computed properties:

- `iconWidth`, `iconHeight` (mode-dependent visual contract)
- `iconBounds` (actual centered icon frame inside the `16 x 16` button)
- `resolvedIconColor` (tone/enabled-aware arrow color)

## Visual Contract

- Fixed frame: `16 x 16` (`Theme.iconSm`)
- Corner radius: `Theme.radiusSm` (`4`)
- Primary tone:
  - background: `Theme.primary`
  - icon: `Theme.accentWhite`
- Borderless tone:
  - background: transparent
  - icon: `Theme.primary`
  - hover/pressed background follow borderless button policy

## Usage

```qml
import LVRS 1.0 as LV

LV.Stepper {
    tone: LV.AbstractButton.Borderless
    arrow: LV.Stepper.Up
    onClicked: console.log("step up")
}
```

## How It Works

- Builds the chevron as an inline SVG snapshot image instead of painting it through `Canvas`.
- Chooses the snapshot profile per target (`desktop` / `mobile`) and requests a supersampled `sourceSize` from `RenderQuality.effectiveSupersampleScaleValue` and `Screen.devicePixelRatio`, so the combo indicator no longer shares a single raster path across platforms.
- Keeps the icon in a device-pixel-snapped centered frame (`10 x 6` or `6.436 x 11.146`) instead of stretching it to the whole button box.
- Uses its own hover/press/disabled mouse pipeline so icon geometry stays isolated from external button templates.
- Rebuilds the inline SVG snapshot automatically when `arrow`, target profile, or resolved icon color changes.

## Practical Notes

- Use `Stepper.UpDown` for generic spinner affordance.
- Use `Stepper.Up`/`Stepper.Down` when separate controls are required.
