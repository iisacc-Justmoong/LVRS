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
- `boxSize` (default `Theme.iconSm`: desktop `18 x 18`, mobile `23 x 23`)
- `boxRadius`, `checkMarkStrokeWidth`, and border widths retain their former 17px-frame ratios as the indicator scales.

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
- The checkbox/checkmark frame follows the shared compact icon size while the Canvas path uses relative coordinates for proportional scaling.
- Checkmark is drawn by `Canvas`, uses a supersampled backing store (`RenderQuality` + HiDPI), rounds each raster axis up independently, and repaints on state/color/stroke changes.
- `showInnerShadow` is disabled only when checked+enabled.

## Usage

```qml
import LVRS 1.0 as LV

LV.CheckBox {
    text: "Remember"
    checked: true
}
```
