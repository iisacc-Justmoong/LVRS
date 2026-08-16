# LabelSegmentedControl

Location: `qml/components/control/buttons/LabelSegmentedControl.qml`

`LabelSegmentedControl` is the Figma label segmented container for `LabelButton` children.

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
- injected method API: `method`, `methods`, `hasInjectedMethods`, `invokeMethods(...)`

## Behavior Contract

- Tone-capable children are treated as segments.
- Tone synchronization (`AbstractButton.Borderless`) runs on completion and on child mutations.
- Child button dimensions remain owned by `LabelButton`; the row is laid out again when segment padding or spacing changes so updated desktop/mobile button metrics propagate to the container.
- The container itself can receive injected methods for direct orchestration; child button clicks remain owned by each child button.

## Figma Visual Contract

- Source: `206:3827` (`LabelSegementedControl`).
- Each segment is the `44:599` borderless `LabelButton`: `56 x 22`, horizontal padding `8`, Body `13px Medium / 13px`.
- Container: horizontal padding `4`, vertical padding `3.5`, spacing `2`, border `2`, radius `8`.
- Surface: `Theme.panelBackground08`; border: `Theme.panelBackground12`.

| Count | Desktop size |
| ---: | ---: |
| 2 | `122 x 29` |
| 3 | `180 x 29` |
| 4 | `238 x 29` |
| 5 | `296 x 29` |
| 6 | `354 x 29` |
| 7 | `412 x 29` |

The desktop width is `count * 56 + (count - 1) * 2 + 8`. With the mobile `2x` token policy, a two-segment control becomes `164 x 58` from two `72 x 44` buttons, `4` spacing, `8` horizontal padding, and `7` vertical padding. Body remains fixed at `13px`.

## Usage

```qml
import LVRS 1.0 as LV

LV.LabelSegmentedControl {
    method: function(eventData) { syncSelectionModel() }

    LV.LabelButton { text: "A" }
    LV.LabelButton { text: "B" }
}
```
