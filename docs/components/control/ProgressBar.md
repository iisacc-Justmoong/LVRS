# ProgressBar

Location: `qml/components/control/display/ProgressBar.qml`

`ProgressBar` is a lightweight range-based progress indicator.

## Purpose

- Render normalized progress from custom numeric ranges.
- Read range state from a C++ `StateModel` when provided.
- Support compact size presets and shape policy.

## Core API

Size and shape:

- `size` (`large`, `regular`)
- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cornerRadius`
- `barHeight` (readonly)

Range:

- `minimumValue`
- `maximumValue`
- `startValue`
- `currentValue`
- `endValue` (compatibility alias for `maximumValue`)
- `stateModel`
- `minimumValueStateKey`
- `maximumValueStateKey`
- `startValueStateKey`
- `currentValueStateKey`
- `usingStateModel` (readonly)
- `effectiveMinimumValue` (readonly)
- `effectiveMaximumValue` (readonly)
- `effectiveStartValue` (readonly)
- `effectiveCurrentValue` (readonly)
- `valueRange` (readonly)
- `normalizedStart` (readonly, clamped to `0..1`)
- `normalizedCurrent` (readonly, clamped to `0..1`)
- `fillStart` (readonly, clamped segment start)
- `fillProgress` (readonly, clamped segment length)
- `progress` (readonly, clamped to `0..1`)

Colors:

- `trackColor`
- `fillColor`

## Behavior Contract

- `minimumValue` and `maximumValue` define the full numeric range.
- `startValue` and `currentValue` define the filled segment inside that range.
- If `stateModel` is present, the bar reads `effective*` values from that C++ state object by key and falls back to the direct QML properties when a key is missing or non-numeric.
- `progress = (effectiveCurrentValue - effectiveMinimumValue) / (effectiveMaximumValue - effectiveMinimumValue)` with clamping.
- The visual fill starts at `fillStart = min(normalizedStart, normalizedCurrent)`.
- The visual fill width is `fillProgress = abs(normalizedCurrent - normalizedStart)`.
- Near-zero range falls back to binary output (`0` or `1`).
- Cylinder shape uses min(width,height)/2 radius.

## Usage

```qml
import LVRS 1.0 as LV

LV.ProgressBar {
    width: 180
    size: regular
    minimumValue: 0
    maximumValue: 100
    startValue: 0
    currentValue: 64
}
```

## C++ State Example

```qml
import LVRS 1.0 as LV

LV.StateModel {
    id: progressState
    values: ({
        minimumValue: 0,
        maximumValue: 100,
        startValue: 0,
        currentValue: 64
    })
}

LV.ProgressBar {
    width: 180
    stateModel: progressState
}
```

## Segment Example

```qml
import LVRS 1.0 as LV

LV.ProgressBar {
    width: 180
    minimumValue: -50
    maximumValue: 150
    startValue: 50
    currentValue: 100
}
```

This fills the segment from 50% to 75% of the track. If `currentValue` is lower than `startValue`, the component still renders the segment between the two values.
