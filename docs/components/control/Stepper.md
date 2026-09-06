# Stepper

Location: `qml/components/control/buttons/Stepper.qml`

`Stepper` is a compact standalone directional control with a square `Theme.iconSm` frame (`18 x 18` on desktop, `36 x 36` on mobile) and `Up`, `Down`, and `UpDown` arrow modes.

## Purpose

- Provide a minimal increment/decrement trigger shape for dense control rows.
- Keep interaction states deterministic across `Primary` and `Borderless` tones.

## Core API

- `tone` (default: `AbstractButton.Primary`)
- `arrow` (default: `Stepper.UpDown`)
  - `Stepper.UpDown`
  - `Stepper.Up`
  - `Stepper.Down`
- `stepped(direction)` emits `1` for increment or `-1` for decrement after a pointer click. `UpDown` uses the upper/lower half; `Up` and `Down` always emit their displayed direction. Disabled controls emit neither `clicked` nor `stepped`.

Computed properties:

- `iconWidth`, `iconHeight` (mode-dependent visual contract)
- `iconBounds` (actual centered icon artwork bounds inside the square button)
- `renderedBackgroundColor` (the color bound to the rendered frame)
- `resolvedIconColor` (enabled-aware arrow color)
- `resolvedIconName` (compatibility name for the `tone` + `arrow` variant)
- `resolvedIconAssetName` (actual shared Figma SVG asset)
- `iconRotation` (`180` for `Up`, otherwise `0`)

Injected methods:

- `method`: one callable injected directly into the stepper
- `methods`: an array of callables or command objects
- `hasInjectedMethods` (readonly)
- `createMethodEvent(triggerName)`
- `invokeMethod(candidate, eventData)`
- `invokeMethods(eventData)`

## Visual Contract

- Fixed frame: `Theme.iconSm` (`18 x 18` on desktop, `36 x 36` on mobile)
- Corner radius: `Theme.radiusSm` (`4` on desktop, `8` on mobile)
- Primary tone:
  - background: `Theme.primary`
  - icon: `Theme.accentWhite`
- Borderless tone:
  - background: transparent
  - icon: `Theme.accentWhite`
  - hover/pressed background follow borderless button policy
- Arrow artwork at the desktop baseline:
  - `Up` / `Down`: `10 x 6`
  - `UpDown`: `6.43604 x 11.1455`
- Desktop and mobile share the same proportions and `18 x 18` `Theme.iconSm` frame.

## Usage

```qml
import LVRS 1.0 as LV

LV.Stepper {
    tone: LV.AbstractButton.Borderless
    arrow: LV.Stepper.Up
    method: function(eventData) { increment() }
}
```

```qml
LV.Stepper {
    arrow: LV.Stepper.UpDown
    onStepped: function(direction) { quantity += direction }
}
```

`ListItem` combines this event with range-clamped quantity state and keyboard Up/Down. Directional pointer behavior is covered by `LVRSTests_list_composites` in addition to the existing Stepper visual contracts.

## How It Works

- Resolves the exact Figma exports from `StepperChevron.svg` and `StepperUpDownChevron.svg`; `Up` rotates the shared single-chevron asset by `180` degrees.
- Requests a supersampled `Image.sourceSize` from `RenderQuality.effectiveSupersampleScaleValue` and `Screen.devicePixelRatio`, so the static SVG stays crisp on HiDPI targets.
- Centers the current Figma node `254:506` artwork without integer-pixel snapping. At the desktop 18px baseline, `iconWidth`, `iconHeight`, and `iconBounds` therefore retain the exact exported fractional geometry.
- Keeps the hover/press/disabled mouse pipeline local to the component, while the rendered artwork comes from the pre-extracted SVG resources.
- Runs injected `method` and `methods` through the shared button method registry whenever `clicked()` is emitted.

## Practical Notes

- Use `Stepper.UpDown` for generic spinner affordance.
- Use `Stepper.Up`/`Stepper.Down` when separate controls are required.
