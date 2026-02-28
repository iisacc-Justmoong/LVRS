# AbstractButton

Location: `qml/components/control/buttons/AbstractButton.qml`

`AbstractButton` is the shared base for LVRS button-family components.

## Purpose

- Unify tone-based color policy (`Primary`, `Default`, `Borderless`, `Destructive`, `Disabled`).
- Centralize interaction gating (`effectiveEnabled`) and focus behavior.
- Provide shared paddings, radius policy, and implicit size baseline.

## Core API

Tone and shape:

- `tone` (`AbstractButton.ButtonTone`)
- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cornerRadius`
- `resolvedCornerRadius` (readonly)

Interaction:

- `effectiveEnabled` (readonly, `enabled && tone !== Disabled`)
- `hoverEnabled`/`focusPolicy` are derived from `effectiveEnabled`

Colors:

- `textColor`, `textColorDisabled`
- `backgroundColor`, `backgroundColorHover`, `backgroundColorPressed`, `backgroundColorDisabled`
- tone-derived readonly colors: `toneTextColor`, `toneBackgroundColor*`

Layout:

- `horizontalPadding`, `verticalPadding`
- `implicitHeight`/`implicitWidth` from content + paddings

## Behavior Contract

- `tone: Disabled` disables interaction even when `enabled: true`.
- `Borderless` tone keeps transparent base fill and uses surface hover/pressed colors.
- A blocking `MouseArea` is installed when `effectiveEnabled == false` to prevent click-through.
- If disabled while focused, the component clears focus.

## Usage

```qml
import LVRS 1.0 as LV

LV.AbstractButton {
    text: "Action"
    tone: LV.AbstractButton.Primary
}
```
