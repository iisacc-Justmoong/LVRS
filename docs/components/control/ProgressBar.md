# ProgressBar

Location: `qml/components/control/display/ProgressBar.qml`

`ProgressBar` is a lightweight range-based progress indicator.

## Purpose

- Render normalized progress from custom numeric ranges.
- Support compact size presets and shape policy.

## Core API

Size and shape:

- `size` (`large`, `regular`)
- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cornerRadius`
- `barHeight` (readonly)

Range:

- `startValue`
- `endValue`
- `currentValue`
- `valueRange` (readonly)
- `progress` (readonly, clamped to `0..1`)

Colors:

- `trackColor`
- `fillColor`

## Behavior Contract

- `progress = (currentValue - startValue) / (endValue - startValue)` with clamping.
- Near-zero range falls back to binary output (`0` or `1`).
- Cylinder shape uses min(width,height)/2 radius.

## Usage

```qml
import LVRS 1.0 as LV

LV.ProgressBar {
    width: 180
    size: regular
    startValue: 0
    endValue: 100
    currentValue: 64
}
```
