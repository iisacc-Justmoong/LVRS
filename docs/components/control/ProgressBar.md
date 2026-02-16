# ProgressBar

Location: `qml/components/control/display/ProgressBar.qml`

`ProgressBar` is a lightweight value-range visualizer for dashboard/status surfaces.

## Purpose

- Render normalized progress from arbitrary numeric range.
- Provide compact size presets with customizable colors.

## API

Size constants:

- `large`
- `regular`

Range/value:

- `startValue`
- `endValue`
- `currentValue`

Visual:

- `trackColor`
- `fillColor`
- `cornerRadius`
- `largeHeight`, `regularHeight`

Computed:

- `barHeight`
- `valueRange`
- `progress` (clamped to `[0, 1]`)

## Usage

```qml
import LVRS 1.0 as LV

LV.ProgressBar {
    width: 200
    size: regular
    startValue: 0
    endValue: 100
    currentValue: 72
}
```

## How It Works

- Progress uses normalized fraction `(current - start) / (end - start)` with clamp.
- Zero/near-zero range falls back to binary result (0 or 1).
- Fill rect width is `track.width * progress`.

## Advanced Example: Non-Zero Range Base

```qml
import LVRS 1.0 as LV

LV.ProgressBar {
    startValue: 40
    endValue: 80
    currentValue: 52
}
```

This renders progress as `(52-40)/(80-40)=0.3`.

## FAQ

Q. Why does fill disappear at low values?  
A. Width is proportional to normalized progress. Near-zero values may render as subpixel and appear invisible.

Q. Can it represent countdown style?  
A. Yes. Set `startValue > endValue` and update `currentValue` accordingly, then verify expected normalization.
