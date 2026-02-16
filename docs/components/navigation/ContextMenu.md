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
- `itemEventTriggered(eventName, payload, index, item)` signal

Layout/visual:

- `itemWidth`
- `itemSpacing`
- `menuColor` (default: `Theme.contextMenuSurface` = `Theme.panelBackground03`)
- `menuOpacity` (default: `1.0`)
- `resolvedMenuColor`
- `dividerColor` (default: `Theme.contextMenuDivider` = `Theme.panelBackground08`)

Behavior:

- `autoCloseOnTrigger`
- `dismissOnGlobalPress`
- `dismissOnGlobalContextRequest`

Methods:

- `openAt(x, y)`
- `openFor(item, x, y)`
- `dismissIfOutsideGlobalEvent(eventData)`
- `triggerEntry(index)`

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
- `selectionDirection` / `direction` / `chevronDirection` (`right|left|up|down`)
- `eventName` / `event` / `action`
- `eventPayload` / `payload`
- `events` (array of string or event object)
- `onTriggered` / `onClicked` / `handler` (function callback)
- `closeOnTrigger` / `autoClose` / `keepOpen` / `preventClose`

Divider detection:

- `type: "divider"` or `divider: true`

## Callback Context

When item callback (`onTriggered`/`onClicked`/`handler`) is provided, `ContextMenu` calls it with one object:

- `index`
- `item`
- `menu`
- `eventName` (first resolved event name)
- `payload` (first resolved payload)
- `emit(eventName, payload)` (runtime custom event emit)
- `close()` (force close)

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
        {
            id: "copy",
            label: "Copy",
            key: "Cmd+C",
            showChevron: false,
            eventName: "menu.copy",
            eventPayload: ({ source: "editor" }),
            onClicked: function(ctx) {
                console.log("callback:", ctx.eventName, JSON.stringify(ctx.payload))
                ctx.emit("analytics.menuClick", ({ id: "copy" }))
            }
        },
        { type: "divider" },
        {
            id: "inspect",
            label: "Inspect",
            showChevron: false,
            events: [
                "menu.inspect",
                { name: "analytics.menuClick", payload: ({ target: "inspect" }) }
            ],
            keepOpen: true
        }
    ]

    onItemEventTriggered: function(eventName, payload, index, item) {
        console.log("event:", eventName, index)
    }
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

- Use entry-level `closeOnTrigger`/`keepOpen` to override close behavior per row.
- Keep divider entries explicit to avoid ambiguity in object-type rows.
- Use `triggerEntry(index)` for programmatic invocation in tests or command routing.

## Failure Pattern

Opening menu with coordinates from local item space without conversion can place popup off-target.
Use `openFor(item, x, y)` or convert coordinates to overlay parent space.
