# CheckBox

Location: `qml/components/control/check/CheckBox.qml`

`CheckBox` is a custom-painted checkbox (`AbstractButton` based) with deterministic state visuals.

## Purpose

- Keep checkbox visuals independent from platform style variance.
- Expose explicit checked/unchecked + enabled/disabled palette and border policy.

## Core API

State:

- `checked` (inherited)
- `enabled` (inherited)
- `text` (inherited)

Shape and metrics:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `boxSize`, `boxRadius`
- `checkMarkStrokeWidth`

Palette and border:

- `checkedColor`, `uncheckedColor`
- `disabledCheckedColor`, `disabledUncheckedColor`
- `checkColor`, `checkMarkColorDisabled`
- `boxBorderWidth*`, `boxBorderColor*`
- `innerShadowSoftColor`, `innerShadowStrongColor`

Resolved values:

- `resolvedCheckedFillColor`, `resolvedUncheckedFillColor`
- `resolvedBoxRadius`
- `resolvedBoxBorderWidth`, `resolvedBoxBorderColor`
- `showInnerShadow`

## Behavior Contract

- `checkable: true`, `tone: Borderless`, transparent background layers.
- Checkmark is drawn by `Canvas` and repainted on state/color/stroke changes.
- `showInnerShadow` is disabled only when checked+enabled.

## Usage

```qml
import LVRS 1.0 as LV

LV.CheckBox {
    text: "Remember"
    checked: true
}
```
