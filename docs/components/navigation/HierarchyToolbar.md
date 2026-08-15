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
- `slotSize`
- `distributeSpacing`
- `backgroundColor`
- `backgroundOpacity`
- `visibleButtonCount`
- `distributedSpacing`

Signals:

- `activeChanged(button, buttonId, index)`
- `buttonTriggered(button, buttonId, index, item)`
- `buttonEventTriggered(eventName, payload, index, item, buttonId)`

Methods:

- `triggerIndex(index)`
- `buttonAt(index)`
- `collectButtons()`

Compatibility:

- `buttons` default property alias is kept for manual children.
- Manual `IconButton` declarations are treated as toolbar slots; declared count equals toolbar button count.
- Manual `IconButton` clicks are auto-wired to toolbar activation and receive default/borderless active tone switching.

## Figma Layout Baseline

Default layout follows the hierarchy header toolbar spec from Figma (`Whatson`, node `134:4111`):

- The desktop toolbar content area is treated as a `200 x 20` reference strip.
- Each toolbar slot uses `Theme.gap20` (`20 x 20` on desktop, `25 x 25` on mobile).
- Each stock icon uses `Theme.iconSm` (`18 x 18` on desktop, `23 x 23` on mobile), leaving the scaled compact inset inside the slot.
- Items are distributed left-to-right across available width (`justify-between` behavior).
- Default padding is zero (`horizontalPadding = 0`, `verticalPadding = 0`).
- Default background is transparent (`backgroundOpacity = 0`).

If fixed-gap behavior is needed, set `distributeSpacing: false` and control spacing via `spacing`.

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
