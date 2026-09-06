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

- `minimumToolbarWidth`
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

Default layout follows `HierarchyToolbar` in Figma (`Layerd Visual Render System`, node `180:1011`):

- The toolbar has a `200 x 26` baseline on desktop and mobile.
- `minimumToolbarWidth` defaults to `200` on desktop and `400` on mobile.
- Default padding is `8px` horizontal and `2px` vertical on desktop (`16px` and `4px` on mobile).
- Each toolbar slot follows the inherited button frame: `22 x 22` on desktop and `44 x 44` on mobile.
- Each stock icon uses `Theme.iconSm`: `18 x 18` on desktop and `36 x 36` on mobile.
- Items are adjacent from the leading inset with no gap. The four reference slots begin at desktop x positions `8`, `30`, `52`, and `74`.
- `distributeSpacing` defaults to `false`; set it to `true` only when a host intentionally needs distributed slots.
- Default background is transparent (`backgroundOpacity = 0`).

For a custom fixed gap, keep `distributeSpacing: false` and set `spacing` explicitly.

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
