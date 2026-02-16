# WheelScrollGuard

Location: `qml/components/control/util/WheelScrollGuard.qml`

`WheelScrollGuard` routes wheel deltas to an intended inner flickable and optionally consumes the event.

## Purpose

- Prevent nested scroll bleed between inner and outer scroll surfaces.
- Convert wheel delta input into bounded `contentY` updates.

## API

- `targetFlickable` (must expose `contentY`, `contentHeight`, `height`)
- `consumeInside` (default `true`)
- `fallbackStep` (default `Theme.gap20`)
- `wheelRouted(wheelEvent, delta, previousContentY, nextContentY)` signal

## Usage

```qml
import LVRS 1.0 as LV

LV.WheelScrollGuard {
    anchors.fill: parent
    targetFlickable: innerFlick
    consumeInside: true
}
```

## How It Works

- Uses internal `EventListener` trigger `wheel`.
- Routes events only when pointer point lies within target flickable bounds.
- Delta source priority: `pixelDelta.y` -> `angleDelta.y` converted by `fallbackStep`.
- Applies bounded `contentY` updates (`0..maxContentY`) and emits `wheelRouted`.

## Advanced Example: Passive Routing Mode

```qml
import LVRS 1.0 as LV

LV.WheelScrollGuard {
    targetFlickable: editorFlick
    consumeInside: false
}
```

This mode routes wheel delta but does not force event acceptance.

## FAQ

Q. Why does wheel scroll still affect outer container?  
A. Check `consumeInside`. When false, event propagation is intentionally allowed.

Q. Why no movement even though wheel events fire?  
A. Verify target exposes valid `contentHeight`, `height`, and currently has scrollable overflow.

## Validation Checklist

- pointer-inside detection maps correctly under nested transforms,
- delta conversion behaves consistently across mouse and touchpad,
- bounded scrolling prevents overshoot past content limits.
