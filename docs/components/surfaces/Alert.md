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
- `appIconSource`, `appIconSize`
- `appIconBackgroundColor`, `appIconFrameColor`, `appIconInnerColor` (empty-source fallback)

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
- Both Figma variants use a `328px` preferred card width, `12px` radius, `32px` top padding, `64px` app icon, `8px` section spacing, and `24px` horizontal/action padding.
- The 2-action variant uses two equal-width horizontal buttons with a `12px` gap; the 3-action variant stacks full-width buttons with the same gap.
- Alert action buttons override the shared `AlertButton` baseline only within `Alert`: vertical padding is `4.5px`, producing a `22px` button at the desktop scale with the `13px` body line box.
- The bundled default app icon is the exact raster asset exported from Figma nodes `106:281` and `106:282`; set `appIconSource: ""` to use the color-configurable fallback.
- The card surface and both title/body text colors map to `Theme.panelBackground07` and `Theme.bodyColor`, respectively.
- Alert title/body blocks normalize Qt glyph bounds to the Figma `22px`/`13px` line-box multiples, keeping the reference variants at `242px` and `310px` tall at desktop scale.

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
