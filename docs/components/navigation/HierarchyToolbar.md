# HierarchyToolbar

Location: `qml/components/navigation/HierarchyToolbar.qml`

`HierarchyToolbar` is the top toolbar for hierarchy panels, rendered as an `IconButton` array.

## Purpose

- Render toolbar buttons from an external array model.
- Keep active slot selection deterministic (`activeIndex`, `activeButtonId`).
- Dispatch id-based and event-based actions from each toolbar slot.

## API

Model and state:

- `buttonItems` (array or model)
- `itemCount`
- `buttonCount`
- `activeButton`
- `activeButtonId`
- `activeIndex`

Layout and appearance:

- `horizontalPadding`
- `verticalPadding`
- `spacing`
- `backgroundColor`
- `backgroundOpacity`

Signals:

- `activeChanged(button, buttonId, index)`
- `buttonTriggered(button, buttonId, index, item)`
- `buttonEventTriggered(eventName, payload, index, item, buttonId)`

Methods:

- `triggerIndex(index)`
- `buttonAt(index)`
- `collectButtons()`

Compatibility:

- `buttons` default property alias is kept for manual `ToolbarButton` children.

## Item Model Contract

Each item can be object or string.

Object fields:

- `id` / `buttonId` / `key`
- `iconName` / `icon` (svg icon name)
- `iconSource` / `source` / `url` (optional direct source)
- `enabled`
- `visible`
- `selected` / `active`
- `eventName` / `event` / `action`
- `eventPayload` / `payload`
- `events` (array of string or event object)
- `onTriggered` / `onClicked` / `handler`

String item:

- treated as `iconName`

## Callback Context

When callback exists, toolbar invokes it with:

- `index`
- `item`
- `button`
- `buttonId`
- `toolbar`
- `eventName`
- `payload`
- `emit(eventName, payload)`
- `activate(index)`

## Usage

```qml
import LVRS 1.0 as LV

LV.HierarchyToolbar {
    id: toolbar
    buttonItems: [
        {
            id: "structure",
            iconName: "projectStructure",
            selected: true,
            eventName: "hierarchy.structure"
        },
        {
            id: "layers",
            iconName: "projectStructure",
            events: [
                "hierarchy.layers",
                { name: "analytics.hierarchy.layers", payload: ({ source: "toolbar" }) }
            ],
            onClicked: function(ctx) {
                ctx.emit("hierarchy.layers.clicked", ({ id: ctx.buttonId }))
            }
        }
    ]
}
```

## Failure Pattern

If `buttonItems` object entries omit both `iconName` and `iconSource`, the slot renders with fallback icon behavior from `IconButton`.
Always provide explicit icon names for predictable visuals.
