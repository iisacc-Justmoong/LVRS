# Stepper

Location: `qml/components/control/buttons/Stepper.qml`

`Stepper` is a compact directional button component that follows the Figma stepper contract (`16x16`, radius `4`) with `Up`, `Down`, and `UpDown` arrow modes.

## Purpose

- Provide a minimal increment/decrement trigger shape for dense control rows.
- Keep interaction states deterministic across `Primary` and `Borderless` tones.

## Core API

- Inherits from `AbstractButton`.
- `tone` (default: `AbstractButton.Primary`)
- `arrow` (default: `Stepper.UpDown`)
  - `Stepper.UpDown`
  - `Stepper.Up`
  - `Stepper.Down`

Computed properties:

- `iconWidth`, `iconHeight` (mode-dependent visual contract)
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

- Renders the Figma chevron as filled `Canvas` paths to avoid external icon dependencies.
- Uses `AbstractButton` interaction pipeline for hover/press/disabled behavior.
- Repaints icon canvas when `arrow` or resolved icon color changes.

## Practical Notes

- Use `Stepper.UpDown` for generic spinner affordance.
- Use `Stepper.Up`/`Stepper.Down` when separate controls are required.
