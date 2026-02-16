# RuntimeEvents

Location: `backend/runtime/runtimeevents.h` / `backend/runtime/runtimeevents.cpp`

`RuntimeEvents` is the runtime daemon singleton that captures interaction and process telemetry and exposes it to QML.

## Purpose

- Capture unified runtime event stream across input/UI/process domains.
- Publish aggregate state as properties for direct binding.
- Provide bounded recent-event history and health snapshots.

## Domain Coverage

Keyboard domain:

- key press/release counts
- last key, text, modifiers
- pressed key set and helper lookups

Pointer domain:

- move/press/release counts
- last mouse position/buttons/modifiers
- press/release elapsed durations
- pointer hit-test UI snapshot (`pointerUi`)

UI lifecycle domain:

- created/shown/hidden/destroyed counts
- last UI event/object/class

Daemon/process domain:

- running state
- idle state and idle timings
- process identity, uptime, rss memory
- recent event ring buffer and sequence

## Core Methods

Lifecycle:

- `start()`
- `stop()`
- `attachWindow(window)`
- `markActivity()`
- `resetCounters()`

Snapshot APIs:

- `snapshot(): map`
- `daemonHealth(): map`
- `inputState(): map`
- `recentEvents(): list`
- `clearRecentEvents()`
- `hitTestUiAt(globalX, globalY): map`

Utility:

- `isKeyPressed(key): bool`

## Signals of Interest

- `eventRecorded(eventData)`
- `daemonHeartbeat(epochMs, uptimeMs, eventSequence)`
- `mousePressed(...)`, `mouseReleased(...)`
- `contextRequested(x, y, modifiers, reason)`
- `uiEvent(eventType, objectName, className, visible)`

## Typical Integration

- `ApplicationWindow` can start/attach runtime monitoring.
- `Backend` hooks `eventRecorded` and mirrors recent events.
- `EventListener` consumes runtime/global signals and resolves payload context.

## Operational Notes

- Recent event capacity is configurable (`recentEventCapacity`).
- High-frequency event emission is internally throttled by minimum interval policies.
- Hit testing falls back gracefully when direct quick-item resolution is unavailable.
- Capture profile can be switched at runtime:
  - `FullCapture` (`captureProfile = 0`)
  - `BalancedCapture` (`captureProfile = 1`)
  - `LowLatencyCapture` (`captureProfile = 2`)
- Low-latency profile lowers runtime signal frequency and disables pointer-hit tracking + UI lifecycle tracking.

## Extended Example: Idle Detection Hook

```qml
import LVRS 1.0 as LV

Connections {
    target: LV.RuntimeEvents
    function onIdleEntered() {
        console.log("user idle", LV.RuntimeEvents.idleForMs)
    }
    function onIdleExited() {
        console.log("user active")
    }
}
```

## Troubleshooting Matrix

- No events recorded: check `start()` and event filter attachment.
- No UI hit data: verify `attachWindow(window)` and visible quick item tree.
- Memory metrics unavailable: platform-specific RSS sampling may be constrained.

## FAQ

Q. Are all OS events mirrored 1:1?  
A. No. Runtime stream is normalized and may coalesce/throttle high-frequency classes.
