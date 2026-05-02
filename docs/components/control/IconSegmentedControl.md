# IconSegmentedControl

Location: `qml/components/control/buttons/IconSegmentedControl.qml`

`IconSegmentedControl` is a segmented container for icon-centric child buttons.

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
- The container can run injected methods through `invokeMethods(...)`; child buttons still own their own click methods.

## Usage

```qml
import LVRS 1.0 as LV

LV.IconSegmentedControl {
    methods: [function(eventData) { syncToolbarState() }]

    LV.IconButton { iconName: "projectStructure" }
    LV.IconButton { iconName: "projectStructure" }
}
```
