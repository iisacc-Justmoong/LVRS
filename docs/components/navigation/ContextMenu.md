# ContextMenu

Location: `qml/components/navigation/ContextMenu.qml`

`ContextMenu` is a popup menu with runtime-tuned open animation and global outside-dismiss bridging.

## Purpose

- Render heterogeneous menu models (string/object/divider).
- Emit normalized event signals and optional callback context.
- Keep close behavior deterministic across overlay/global-event paths.

## Core API

Data and selection:

- `items`
- `selectedIndex`
- `entryCount` (readonly)

Open/close behavior:

- `autoCloseOnTrigger`
- `dismissOnGlobalPress`
- `dismissOnGlobalContextRequest`
- `openAt(x, y)`
- `openFor(item, x, y)`
- `dismissIfOutsideGlobalEvent(eventData)`
- `triggerEntry(index)`

Visual/layout:

- `itemWidth`, `itemSpacing`
- `menuColor`, `menuOpacity`, `resolvedMenuColor`
- `dividerColor`

Animation tuning:

- `enableOpenBounce`, `autoTuneByBackend`
- `openBounceDuration`, `openSettleDuration`
- `openStartScale`, `openOvershootScale`
- resolved: `resolvedOpenBounceEnabled`, `resolvedOpenReboundDuration`, `resolvedOpenBackOvershoot`

Signals:

- `itemTriggered(index, item)`
- `itemEventTriggered(eventName, payload, index, item)`

## Entry Object Contract

Supported object fields include:

- label/text: `label`, `text`, `title`
- icon: `iconName`/`icon`, `iconSource`/`source`
- key text: `key`, `shortcut`, `keyText`
- key visibility: `keyVisible`, `shortcutVisible`, `showShortcut`
- state: `enabled`, `state`, `selected`
- chevron/children: `showChevron`, `hasChildItems`, `hasSubmenu`, `expanded`, `selectionDirection`
- event: `eventName`/`event`/`action`, `eventPayload`/`payload`, `events[]`
- callback: `onTriggered`, `onClicked`, `handler`
- close policy: `closeOnTrigger`, `autoClose`, `keepOpen`, `preventClose`
- divider: `type: "divider"` or `divider: true`

Callback receives context `{ index, item, menu, eventName, payload, emit(), close() }`.

Chevron render condition is `showChevron && hasChildItems` (resolved from entry fields).

## Usage

```qml
import LVRS 1.0 as LV

LV.ContextMenu {
    id: menu
    items: [
        { label: "Copy", eventName: "menu.copy", showChevron: false },
        { type: "divider" },
        { label: "Inspect", keepOpen: true, events: ["menu.inspect"] }
    ]
}
```
