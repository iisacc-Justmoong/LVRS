# Alert

Location: `qml/components/surfaces/Alert.qml`

`Alert` is an overlay dialog surface with 1/2/3-action layouts.

## Purpose

- Provide centralized alert UI with explicit action layout rules.
- Support overlay-layer reparenting and optional backdrop dismiss.

## Core API

State and content:

- `open`
- `title`, `message`
- `buttonCount` (`0=auto`, `2`, `3`)
- `primaryText`, `secondaryText`, `tertiaryText`
- `primaryEnabled`, `secondaryEnabled`, `tertiaryEnabled`

Behavior:

- `dismissOnBackground`
- `useOverlayLayer`

Sizing and style:

- `minWidth`, `maxWidth`, `preferredWidth`
- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cardCornerRadius`, `resolvedCardCornerRadius`
- `backdropColor`, `cardBackgroundColor`
- `appIconBackgroundColor`, `appIconFrameColor`, `appIconInnerColor`

Resolved layout flags:

- `resolvedButtonCount`
- `hasSecondaryAction`, `hasTertiaryAction`
- `useVerticalActionLayout`

Signals:

- `primaryClicked()`
- `secondaryClicked()`
- `tertiaryClicked()`
- `dismissed()`

## Layout Rules

- explicit `buttonCount: 3` -> vertical 3-button layout
- explicit `buttonCount: 2` -> horizontal 2-button layout
- `buttonCount: 0` (auto):
  - tertiary text exists -> vertical layout
  - secondary text exists -> horizontal 2-button layout
  - otherwise -> single primary button
- Alert action buttons override the shared `AlertButton` baseline only within `Alert`: top/bottom padding uses `Theme.gap8`, and the resolved button height expands with that padding so the body label line box is not clipped.

## Behavior Contract

- On open, component can reparent to `Controls.Overlay.overlay`.
- Backdrop click closes only when `dismissOnBackground == true`.
- Action buttons emit signals only; close behavior is controlled by caller state.

## Usage

```qml
import LVRS 1.0 as LV

LV.Alert {
    open: true
    buttonCount: 2
    title: "Delete item?"
    message: "This action cannot be undone."
    primaryText: "Delete"
    secondaryText: "Cancel"
}
```
