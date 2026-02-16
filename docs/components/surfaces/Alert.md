# Alert

Location: `qml/components/surfaces/Alert.qml`

`Alert` is a centered overlay dialog surface with configurable 1/2/3 action layouts.

## Purpose

- Provide app-level blocking/attention dialog.
- Support primary/secondary/tertiary action patterns in one component.

## API

State and text:

- `open`
- `title`
- `message`
- `primaryText`, `secondaryText`, `tertiaryText`
- `primaryEnabled`, `secondaryEnabled`, `tertiaryEnabled`

Behavior:

- `dismissOnBackground`
- `useOverlayLayer`

Sizing/visual:

- `minWidth`, `maxWidth`, `preferredWidth`
- `backdropColor`
- `cardBackgroundColor`
- `appIconBackgroundColor`, `appIconFrameColor`, `appIconInnerColor`

Derived layout flags:

- `hasSecondaryAction`
- `hasTertiaryAction`
- `useVerticalActionLayout`

Signals:

- `primaryClicked()`
- `secondaryClicked()`
- `tertiaryClicked()`
- `dismissed()`

## Action Layout Rules

- tertiary present -> vertical action stack (up to 3 buttons)
- tertiary absent + secondary present -> horizontal two-button row
- only primary -> single full-width primary button

## Usage

```qml
import LVRS 1.0 as LV

LV.Alert {
    open: state.showDeleteDialog
    title: "Delete Scene?"
    message: "This action cannot be undone."
    primaryText: "Delete"
    secondaryText: "Cancel"
    onPrimaryClicked: confirmDelete()
    onSecondaryClicked: state.showDeleteDialog = false
}
```

## How It Works

- When open, component re-parents to overlay layer when configured.
- Backdrop click dismiss behavior is opt-in (`dismissOnBackground`).
- Card background defaults differ by action layout to keep contrast stable.

## Advanced Example: Three-Action Vertical Layout

```qml
import LVRS 1.0 as LV

LV.Alert {
    open: true
    title: "Unsaved Changes"
    message: "Choose how to proceed."
    primaryText: "Save"
    secondaryText: "Discard"
    tertiaryText: "Cancel"
}
```

## Operational Notes

- `tertiaryText` presence switches action area to vertical layout.
- Use `dismissed()` to unify background-dismiss state cleanup.
- Keep `open` as single source of truth in app state store to avoid stale overlay visibility.

## Failure Pattern

Maintaining separate local `open` flags in multiple components can desynchronize dialog state.
Use a single app-level source of truth for alert visibility.
