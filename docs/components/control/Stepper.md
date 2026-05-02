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
- `resolvedIconColor` (enabled-aware arrow color)
- `resolvedIconName` (maps `tone` + `arrow` to the shipped static SVG asset)

Injected methods:

- `method`: one callable injected directly into the stepper
- `methods`: an array of callables or command objects
- `hasInjectedMethods` (readonly)
- `createMethodEvent(triggerName)`
- `invokeMethod(candidate, eventData)`
- `invokeMethods(eventData)`

## Visual Contract

- Fixed frame: `16 x 16` (`Theme.iconSm`)
- Corner radius: `Theme.radiusSm` (`4`)
- Primary tone:
  - background: `Theme.primary`
  - icon: `Theme.accentWhite`
- Borderless tone:
  - background: transparent
  - icon: `Theme.accentWhite`
  - hover/pressed background follow borderless button policy

## Usage

```qml
import LVRS 1.0 as LV

LV.Stepper {
    tone: LV.AbstractButton.Borderless
    arrow: LV.Stepper.Up
    method: function(eventData) { increment() }
}
```

## How It Works

- Resolves the exact shipped Figma export from `resources/iconset/Stepper*.svg` instead of rebuilding the chevrons procedurally at runtime.
- Requests a supersampled `Image.sourceSize` from `RenderQuality.effectiveSupersampleScaleValue` and `Screen.devicePixelRatio`, so the static SVG stays crisp on HiDPI targets.
- Preserves the public chevron geometry contract (`10 x 6` or `6.436 x 11.146`) through `iconWidth`, `iconHeight`, and `iconBounds`.
- Keeps the hover/press/disabled mouse pipeline local to the component, while the rendered artwork comes from the pre-extracted SVG resources.
- Runs injected `method` and `methods` through the shared button method registry whenever `clicked()` is emitted.

## Practical Notes

- Use `Stepper.UpDown` for generic spinner affordance.
- Use `Stepper.Up`/`Stepper.Down` when separate controls are required.
