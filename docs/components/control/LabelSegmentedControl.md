# LabelSegmentedControl

Location: `qml/components/control/buttons/LabelSegmentedControl.qml`

`LabelSegmentedControl` is a segmented container for text-oriented child buttons.

## Purpose

- Provide the same segmented shell behavior as `IconSegmentedControl` for label buttons.
- Keep segmented tone policy and sizing consistent.

## Core API

Container style:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cornerRadius`, `resolvedCornerRadius`
- `horizontalPadding`, `verticalPadding`, `spacing`
- `borderWidth`, `borderColor`
- `backgroundColor`

Behavior:

- `forceBorderlessTone` (default `true`)
- `segmentCount` (readonly)
- `default property alias buttons: segmentRow.data`

## Behavior Contract

- Tone-capable children are treated as segments.
- Tone synchronization (`AbstractButton.Borderless`) runs on completion and on child mutations.

## Usage

```qml
import LVRS 1.0 as LV

LV.LabelSegmentedControl {
    LV.LabelButton { text: "A" }
    LV.LabelButton { text: "B" }
}
```
