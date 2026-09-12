# Modal

Location: `qml/components/surfaces/Modal.qml`

`Modal` is an Apple-style dialog surface with left icon, title/description, and up to three action buttons.

## Purpose

- Provide a centered top-shifted modal frame with icon + text + action controls.
- Show a dimmed overlay while modal is open.
- Cancel when the user clicks outside the modal frame.

## Core API

State and behavior:

- `open`
- `dismissOnBackground`
- `useOverlayLayer`
- `cancel()`
- `handleBackdropClick(localX, localY)`
- `containsFramePoint(localX, localY)`
- `actionVisible(index)`
- `triggerAction(index)`

Sizing and layout:

- `minWidth`, `maxWidth`, `preferredWidth`
- `sidePadding`
- `frameMinHeight`
- `verticalOffset` (default top-shifted)

Content:

- `title`
- `description`, `message`, `desc` (alias of `description`)
- `iconName`, `iconSource`, `showIcon`
- `iconSize`, `iconCornerRadius`
- `contentComponent`: optional `Component` loaded between the header and actions while open.
- `contentItem`: read-only loaded item, or `null` while closed.
- resolved: `resolvedIconSource`, `resolvedDescription`

Actions:

- `buttonCount` (`0=auto`, `1~3=explicit`)
- `primaryText`, `secondaryText`, `tertiaryText`
- `primaryEnabled`, `secondaryEnabled`, `tertiaryEnabled`
- resolved: `resolvedButtonCount`, `hasPrimaryAction`, `hasSecondaryAction`, `hasTertiaryAction`

Frame style:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `frameCornerRadius`, `resolvedFrameCornerRadius`
- `frameColor`
- `backdropColor`

Signals:

- `canceled()`
- `primaryClicked()`
- `secondaryClicked()`
- `tertiaryClicked()`

## Behavior Contract

- When `open` is `true`, `Modal` is visible and enabled.
- Frame is horizontally centered and slightly above vertical center.
- Backdrop click outside frame calls `cancel()` when `dismissOnBackground == true`.
- Backdrop click inside frame does not cancel.
- When open, component can reparent to `Controls.Overlay.overlay` if available.
- A supplied content view receives the frame's content width and contributes its `implicitHeight`. Closing unloads it; reopening creates a fresh instance. The default icon/text/action layout is unchanged when no component is supplied.
- For a view with its own actions, set `showIcon: false` and `primaryText: ""`. See [ColorPicker](../control/ColorPicker.md) for a complete host example.
- `buttonCount` is clamped to a maximum of 3.
- In auto mode (`buttonCount=0`), tertiary text enables 3 actions, secondary text enables 2, otherwise primary only.

## Usage

```qml
import LVRS 1.0 as LV

LV.Modal {
    open: true
    iconName: "projectStructure"
    title: "Unlock iPhone 15 Pro Max to Continue"
    description: "Xcode cannot launch because the device is locked."
    buttonCount: 2
    primaryText: "Cancel Running"
    secondaryText: "Later"
}
```

## Shared motion

The frame and backdrop enter together; the closing frame remains visible while fading. See [motion policy](../../motion.md) for global speed, reduced motion, local overrides and the component-specific VisualCatalog recipe.
