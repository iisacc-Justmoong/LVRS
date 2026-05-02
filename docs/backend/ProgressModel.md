# ProgressModel

Location: `backend/model/progressmodel.h`, `backend/model/progressmodel.cpp`

`ProgressModel` owns numeric range normalization for `ProgressBar.qml`.

## Purpose

- Keep progress range state and segment math out of QML.
- Read optional `StateModel` values for minimum, maximum, start, and current values.
- Provide clamped normalized values for rendering.
- Centralize radius selection for round-rect versus cylinder progress shapes.

## API

Input:

- `minimumValue`
- `maximumValue`
- `startValue`
- `currentValue`
- `stateModel`
- `minimumValueStateKey`
- `maximumValueStateKey`
- `startValueStateKey`
- `currentValueStateKey`

Readonly:

- `usingStateModel`
- `stateRevision`
- `effectiveMinimumValue`
- `effectiveMaximumValue`
- `effectiveStartValue`
- `effectiveCurrentValue`
- `valueRange`
- `normalizedStart`
- `normalizedCurrent`
- `fillStart`
- `fillProgress`
- `progress`

Methods:

- `stateNumber(key, fallbackValue)`
- `normalizedValue(value)`
- `radiusFor(shapeStyle, cornerRadius, rectWidth, rectHeight)`

## How It Works

- Direct QML properties are always retained as fallbacks.
- If `stateModel` is present, the effective values are read through `StateModel::valueOr`.
- Non-numeric or non-finite state values fall back to the direct property value.
- `normalizedValue` clamps into `0..1`.
- Near-zero range renders as binary progress: current value at or above maximum becomes `1`, otherwise `0`.
- `fillStart` is the lower of normalized start/current, and `fillProgress` is their absolute distance.

## QML Boundary

`ProgressBar.qml` binds visual inputs into `ProgressModel` and renders only the track/fill rectangles. Numeric state resolution, range coercion, normalized progress, segment calculation, and shape radius selection belong to `ProgressModel`.
