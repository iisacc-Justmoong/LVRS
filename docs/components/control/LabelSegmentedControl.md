# LabelSegmentedControl

Location: `qml/components/control/buttons/LabelSegmentedControl.qml`

`LabelSegmentedControl` is the Figma label segmented container for `LabelButton` children.

## Purpose

- Provide the same segmented shell behavior as `IconSegmentedControl` for label buttons.
- Keep segmented tone policy and sizing consistent.

## Core API

Container style:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cornerRadius` (default `Theme.radiusLg`, `12`), `resolvedCornerRadius`
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

- Source: [Figma `206:3827` (`LabelSegementedControl`)](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=206-3827), verified 2026-09-10.
- Each segment is the `44:599` borderless `LabelButton`: `56 x 22`, horizontal padding `8`, Body `13px Medium / 13px`.
- Container: horizontal padding `4`, vertical padding `3.5`, spacing `2`, border `2`, radius `12` (`Theme.radiusLg`). Child buttons retain radius `8` (`Theme.radiusMd`).
- Surface: `Theme.panelBackground08`; border: `Theme.panelBackground12`.

| Count | Desktop size |
| ---: | ---: |
| 2 | `122 x 29` |
| 3 | `180 x 29` |
| 4 | `238 x 29` |
| 5 | `296 x 29` |
| 6 | `354 x 29` |
| 7 | `412 x 29` |

Desktop and mobile use `count * 56 + (count - 1) * 2 + 8` width. A two-segment control is `122 x 29`, with two `56 x 22` buttons, `2` spacing, `4` horizontal padding, and `3.5` vertical padding. Body remains `13px`.

## Verification

`LVRSTests_import_api::segmented_control_figma_contract_loads` checks all six counts under desktop and mobile theme settings, including container/button radii, bounds, tone, padding, spacing, and colors. Set `LVRS_SEGMENT_CAPTURE_DIR` when running it with a native Qt platform to save rendered reference images. The mobile row checks the shared theme contract on the host; it is not a device run.

## Usage

```qml
import LVRS 1.0 as LV

LV.LabelSegmentedControl {
    method: function(eventData) { syncSelectionModel() }

    LV.LabelButton { text: "A" }
    LV.LabelButton { text: "B" }
}
```
