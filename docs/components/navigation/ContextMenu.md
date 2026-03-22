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
- `resolvedItemWidth` (readonly)
- `menuColor`, `menuOpacity`, `resolvedMenuColor`
- `dividerColor`
- `edgeMargin` (viewport inset used by auto placement; default `Theme.gap4`)
- `openHorizontalDirection`, `openVerticalDirection` (last resolved placement direction)

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
Shortcut visibility defaults to `true` only when shortcut text exists; entries without `key`/`shortcut`/`keyText` do not reserve trailing shortcut space unless explicitly requested.

## Placement Contract

- `openAt(x, y)` performs built-in edge-aware placement against overlay bounds.
- Default preference is right/down from anchor point.
- If space is insufficient, placement flips to left and/or up.
- Final position is clamped to viewport with `edgeMargin`.
- `resolveOpenPlacement(...)` exposes the internal placement solver for deterministic testing and tooling.

## Width Contract

- `itemWidth` is the baseline minimum row width.
- `resolvedItemWidth` expands to the larger of baseline `itemWidth`, the widest delegate implicit width, and any explicit popup width supplied by the caller.
- The popup frame itself is promoted to at least `implicitWidth`, so a narrow explicit `width` cannot clamp the menu below its content-driven size.
- Delegate rows and dividers consume `resolvedItemWidth`, so a combo-triggered menu can grow wider than the trigger itself when content or caller sizing requires it.
- Delegate `MenuItem` rows remain responsive inside `resolvedItemWidth`, so constrained menus do not push label/shortcut/chevron content outside the popup bounds or collapse the internal spacer into negative geometry.
- Width probing uses each row's unconstrained natural content width, so visible text elision does not feed back into popup sizing.

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
