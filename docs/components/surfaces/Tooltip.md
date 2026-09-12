# Tooltip

Location: `qml/components/surfaces/Tooltip.qml`

`LV.Tooltip` is a nonmodal speech-bubble tooltip that accepts a reusable QML `Component`, inline items, or ordinary `text`. It extends Qt Quick Controls `ToolTip`; no additional third-party dependency is required. Qt Quick Shapes draws the rounded body and tail as one vector outline.

## Usage

```qml
LV.LabelButton { id: helpButton; text: "Help" }

Component {
    id: helpView
    Item {
        implicitWidth: 220
        implicitHeight: column.implicitHeight
        Column {
            id: column
            width: parent.width
            spacing: LV.Theme.gap8
            LV.Label { style: header2; text: "Version history" }
            LV.Label {
                width: parent.width
                style: body
                text: "Restore an earlier version without losing your current work."
                wrapMode: Text.WordWrap
                sizeToContentHeight: true
            }
        }
    }
}

LV.Tooltip {
    target: helpButton
    contentComponent: helpView
}
```

Set the content root's `implicitWidth` and `implicitHeight` (or use a layout such as Column that supplies them). The host assigns its available width and handles long content with a vertical Flickable. Use `scrollContent: false` when the supplied view, such as a ListView, owns scrolling and should fill the available height. `loadedContent` exposes the Loader-created instance; replacing `contentComponent` replaces that instance. Inline content is used when no Component is supplied; `text` is the fallback when both are empty.

## Origin and placement

- `target`: the item that owns the origin and automatic interaction; optional for manual presentation.
- `anchorPoint`: a point in target-local logical coordinates, defaulting to its center. Without a target, coordinates are relative to the window overlay.
- `openAt(item, point, component)`: set an origin and optionally a Component, then open. Set `automatic: false; delay: 0` for pointer/canvas previews that manage their own lifetime.
- `preferredPlacement`: `Automatic`, `Above`, `Below`, `LeftSide`, or `RightSide`. A preference falls back when it does not fit. The side names avoid Qt Popup's unrelated `Left`/`Right` transform-origin constants.
- `maximumWidth` / `maximumHeight`: body size limits, both `320` by default.
- `availableRect`: the permitted overlay-space viewport, defaulting to the host window's safe area. The viewport is intersected with the current display; a custom rectangle can confine the bubble to a frame.
- `edgeMargin`: `8`; `contentPadding`: `12`; `cornerRadius`: `12`; `tailLength`: `10`; `tailWidth`: `16`.
- `surfaceColor`, `borderColor`: LVRS theme colors.

The preferred direction that fits wins. If none fits the natural body, the direction showing the greatest body area wins and the content viewport is constrained. The body shifts away from edges while the tail's tip stays at the exact origin; its base slides along the body's straight edge. Parent transforms, scrolling, target movement, and window resizing are tracked while visible. A hidden, disabled, detached, or off-viewport target closes its tooltip, including an origin clipped by a scrolling ancestor.

`resolvedPlacement`, `resolvedAnchorPoint`, `bodyRect`, `tailPoint`, `availableContentWidth`, and `availableContentHeight` expose the resolved geometry. The anchor is in overlay coordinates; body/tail coordinates are local to the popup. All dimensions are logical pixels on desktop and mobile.

## Standard interaction

`automatic: true` shows on hover, keyboard focus, or holding the target. The native `delay` defaults to `500ms`; releasing/leaving before it expires cancels display. `timeout` defaults to `5000ms`; `-1` keeps it open until dismissed. A timeout or Escape does not immediately reopen a tooltip while the same trigger remains active.

Leaving the trigger closes after `hideDelay` (`120ms`), allowing the pointer to move into a rich tooltip. The body can host interactive controls. Tooltip presentation does not dim the application, block outside interaction, or take keyboard focus. Escape and outside presses dismiss it. `animationDuration` (`120ms`, or `0` for no animation) controls a short fade. `open()`, `close()`, and the inherited ToolTip delay/timeout API remain available; `automatic: false` allows application-driven visibility.

This is a local `LV.Tooltip` instance, not a replacement for Qt's shared `ToolTip` attached properties. For arbitrary content, use `contentComponent` or inline items.

## Examples and validation

Visual Catalog → Surfaces → Tooltip contains four frame-corner examples, a movable origin, a rich status view with LVRS controls, and automatic text help.

`ctest --test-dir build -R LVRSTests_tooltip --output-on-failure` checks directional fallback, edge shifting and the rendered tail, Component replacement, inline content, constrained scrolling, transformed/moving targets, resize, hover delay, timeout, Escape, touch hold, and nonmodal input. Set `LVRS_TOOLTIP_CAPTURE_DIR` to a directory under `build/` for a gallery capture; use the native renderer to inspect the vector outline.

References: [Qt ToolTip](https://doc.qt.io/qt-6.8/qml-qtquick-controls-tooltip.html), [Qt Quick Shapes](https://doc.qt.io/qt-6.8/qml-qtquick-shapes-shape.html).

## Glass 25 background

The existing body-and-tail outline now masks `PanelMaterial`: 25% neutral tint, 16px diffusion, and the app Primary radial color at 40% / 11% strength. `surfaceColor` defaults to `Theme.materialTint`; `surfaceOpacity` defaults to 0.25. `primaryColor`, `backdropSource` and `backdropBackground` can be supplied for custom hosts. Placement, content replacement, scrolling and dismissal behavior are unchanged. See [Materials](Materials.md).

## Shared motion

The bubble grows from 94% and rebounds while placement and the tail continue tracking the origin. See [motion policy](../../motion.md) for global speed, reduced motion, local overrides and the component-specific VisualCatalog recipe.
