# Alert

Location: `qml/components/surfaces/Alert.qml`

`Alert` is a centered overlay dialog surface with a single opaque card body.

## Visual Structure

- Backdrop layer (`backdropColor`) with optional outside-dismiss (`dismissOnBackground`).
- Single card surface (`cardBackgroundColor`) without outer frame/inner-frame split.
- Content stack:
  - App icon block
  - Title/message text block
  - Action block

The card background defaults are tied to action layout:
- 3-action (vertical): `Theme.panelBackground07`
- 1/2-action (horizontal/single): `Theme.panelBackground08`

This avoids transparency/framing artifacts and keeps dialog contrast stable.

## Properties

State and text:
- `open`, `title`, `message`
- `primaryText`, `secondaryText`, `tertiaryText`
- `primaryEnabled`, `secondaryEnabled`, `tertiaryEnabled`

Behavior and sizing:
- `dismissOnBackground`
- `useOverlayLayer`
- `minWidth`, `maxWidth`
- `preferredWidth` (readonly, fixed to 328)
- `useVerticalActionLayout` (readonly: true when tertiary action exists)

Visual:
- `backdropColor`
- `cardBackgroundColor`
- `appIconBackgroundColor`, `appIconFrameColor`, `appIconInnerColor`

## Signals

- `primaryClicked()`
- `secondaryClicked()`
- `tertiaryClicked()`
- `dismissed()`

## Action Layout Rules

- Tertiary action exists: render vertical 3-button stack.
- Secondary action exists (without tertiary): render horizontal 2-button row.
- Only primary action exists: render one full-width button.

`AlertButton` tones are mapped as:
- Primary action: `AbstractButton.Primary`
- Secondary/tertiary actions: `AbstractButton.Default`

## Usage

```qml
import LVRS 1.0 as LV

LV.Alert {
    open: appState.alertOpen
    title: "Delete Scene?"
    message: "This action cannot be undone."
    primaryText: "Delete"
    secondaryText: "Cancel"
    onPrimaryClicked: appState.confirmDelete()
    onSecondaryClicked: appState.alertOpen = false
}
```
