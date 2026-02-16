# ContextMenu

Location: `qml/components/navigation/ContextMenu.qml`

`ContextMenu` is a popup menu with runtime-aware outside-dismiss bridges.

## Purpose

- Render menu entries from flexible model types.
- Provide robust close behavior under nested overlay/event contexts.

## API

Data/selection:

- `items`
- `selectedIndex`
- `itemTriggered(index, item)` signal

Layout/visual:

- `itemWidth`
- `itemSpacing`
- `menuColor`
- `menuOpacity`
- `resolvedMenuColor`
- `dividerColor`

Behavior:

- `autoCloseOnTrigger`
- `dismissOnGlobalPress`
- `dismissOnGlobalContextRequest`

Methods:

- `openAt(x, y)`
- `openFor(item, x, y)`
- `dismissIfOutsideGlobalEvent(eventData)`

## Entry Model Rules

Each entry can be primitive string or object map.

Object fields (common):

- `label/text/title`
- `key/shortcut`
- `iconName/icon`
- `iconSource/source`
- `enabled`
- `state`
- `selected`
- `showChevron` / `hasSubmenu`

Divider detection:

- `type: "divider"` or `divider: true`

## Dismiss Strategy

Two paths are used together:

1. Popup close policy (`CloseOnPressOutside` etc.).
2. Global event bridge (`EventListener` with `globalPressed`/`globalContextRequested`) calling `dismissIfOutsideGlobalEvent`.

This dual path keeps close behavior deterministic across layered overlays.

## Usage

```qml
import LVRS 1.0 as LV

LV.ContextMenu {
    id: menu
    items: [
        { id: "copy", label: "Copy", key: "Cmd+C", showChevron: false },
        { type: "divider" },
        { id: "inspect", label: "Inspect", showChevron: false }
    ]
}
```

## Advanced Example: Open Near Cursor with Outside Dismiss

```qml
import LVRS 1.0 as LV

LV.ContextMenu {
    id: menu
    dismissOnGlobalPress: true
    dismissOnGlobalContextRequest: true
}

function openMenu(eventData) {
    menu.openAt(eventData.globalX, eventData.globalY)
}
```

## Practical Notes

- Use `autoCloseOnTrigger=false` for submenu-launching rows.
- Keep divider entries explicit to avoid ambiguity in object-type rows.

## Failure Pattern

Opening menu with coordinates from local item space without conversion can place popup off-target.
Use `openFor(item, x, y)` or convert coordinates to overlay parent space.
