# IconSegmentedControl

Location: `qml/components/control/buttons/IconSegmentedControl.qml`

`IconSegmentedControl` is the Figma icon segmented container for `IconButton` children.

## Purpose

- Provide shared segmented shell (background, border, padding, spacing).
- Normalize child button tones when needed.

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

- Children that expose `tone` are treated as segment buttons.
- When `forceBorderlessTone == true`, each segment tone is synchronized to `AbstractButton.Borderless`.
- Child changes are synchronized lazily via `Qt.callLater` (`scheduleSyncSegmentStyles`).
- Child button dimensions remain owned by `IconButton`; segment token changes force a row relayout so the container follows updated desktop/mobile button metrics.
- The container can run injected methods through `invokeMethods(...)`; child buttons still own their own click methods.

## Figma Visual Contract

- Source: `206:3912` (`IconSegementedControl`).
- Each segment is the `44:599` borderless `IconButton`: `22 x 22` with an `18 x 18` icon and `2` inset.
- Container: horizontal/vertical padding `4`, spacing `2`, border `2`, radius `8`.
- Surface: `Theme.panelBackground08`; border: `Theme.panelBackground12`.

| Count | Desktop size |
| ---: | ---: |
| 2 | `54 x 30` |
| 3 | `78 x 30` |
| 4 | `102 x 30` |
| 5 | `126 x 30` |
| 6 | `150 x 30` |
| 7 | `174 x 30` |

Desktop and mobile use `count * 22 + (count - 1) * 2 + 8` width. A two-segment control is `54 x 30`, with two `22 x 22` buttons, `2` spacing, and `4` padding.

## Usage

```qml
import LVRS 1.0 as LV

LV.IconSegmentedControl {
    methods: [function(eventData) { syncToolbarState() }]

    LV.IconButton { iconName: "projectStructure" }
    LV.IconButton { iconName: "projectStructure" }
}
```
