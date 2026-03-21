# RuntimeEvents

Location: `backend/runtime/runtimeevents.h` / `backend/runtime/runtimeevents.cpp`

`RuntimeEvents` is the low-level runtime daemon singleton for LVRS input/UI/process telemetry.  
It is designed to provide observability while giving explicit controls for latency-vs-detail tradeoffs.

## 1. Scope and Responsibilities

`RuntimeEvents` owns:

- application-level event-filter based capture (keyboard/pointer/touch/tablet/gesture/context/UI lifecycle),
- process health signals (uptime, RSS, active state, heartbeat),
- normalized runtime event stream (`eventRecorded` + ring buffer),
- derived snapshots (`snapshot()`, `daemonHealth()`, `inputState()`),
- capture cost controls (`captureProfile`, high-frequency throttles, pointer hit-test gating).

`RuntimeEvents` does **not** own:

- persistence/export policy (`Backend` owns mirrored cache/export),
- rendering-quality policy (`RenderQuality` owns this),
- UI layout semantics (`ApplicationWindow`/QML owns this).

## 2. Property API (Detailed Contract)

### 2.1 Runtime lifecycle

| Property | Type | Semantics |
|---|---|---|
| `running` | `bool` | True when global event filter and timers are active. |
| `daemonBootEpochMs` | `qint64` | Epoch-ms at daemon construction time. |
| `eventSequence` | `quint64` | Monotonic sequence number for recorded runtime events. |
| `lastEvent` | `map` | Last recorded event payload. |

### 2.2 Keyboard domain

| Property | Type | Semantics |
|---|---|---|
| `keyPressCount`, `keyReleaseCount` | `quint64` | Total key press/release counts observed by filter. |
| `lastKey`, `lastKeyText`, `lastKeyModifiers` | `int/string/int` | Last key metadata. |
| `anyKeyPressed` | `bool` | True if pressed-key set is non-empty. |
| `pressedKeys`, `pressedKeyCodes` | `list/list` | Human labels + raw key codes currently pressed. |

### 2.3 Pointer domain

| Property | Type | Semantics |
|---|---|---|
| `mouseMoveCount`, `mousePressCount`, `mouseReleaseCount` | `quint64` | Pointer event counters. |
| `lastMouseX`, `lastMouseY` | `qreal` | Last global pointer coordinates. |
| `lastMouseButtons`, `lastMouseModifiers` | `int` | Last pointer button/modifier state. |
| `mouseButtonPressed` | `bool` | True while active button press is tracked. |
| `pointerUi` | `map` | UI hit-test snapshot or fallback map. |
| `lastMousePressEpochMs`, `lastMouseReleaseEpochMs` | `qint64` | Epoch timestamps of last press/release. |
| `mousePressElapsedMs`, `mouseReleaseElapsedMs` | `qint64` | Elapsed time since last press/release (`-1` if unavailable). |
| `activePressDurationMs` | `qint64` | Duration of currently active press; `0` when no active press. |
| `pressedMouseButtonNames` | `list` | Button labels for current button bitmask. |
| `activeModifiers`, `activeModifierNames` | `int/list` | Modifier union from mouse + keyboard state. |

### 2.4 UI lifecycle domain

| Property | Type | Semantics |
|---|---|---|
| `uiCreatedCount`, `uiShownCount`, `uiHiddenCount`, `uiDestroyedCount` | `quint64` | UI lifecycle counters for tracked tree. |
| `lastUiEvent`, `lastUiObjectName`, `lastUiClassName` | `string` | Last tracked UI lifecycle metadata. |

### 2.5 Idle/process/OS domain

| Property | Type | Semantics |
|---|---|---|
| `idle` | `bool` | Idle state flag. |
| `idleTimeoutMs` | `int` | Idle threshold. Clamped `[250, 86,400,000]`. |
| `idleForMs` | `qint64` | Current idle duration. |
| `lastActivityEpochMs` | `qint64` | Last activity timestamp. |
| `pid`, `processName`, `osName` | scalar | Process/platform identity metadata. |
| `applicationActive` | `bool` | Current app active-state mirror. |
| `osSampleIntervalMs` | `int` | OS telemetry sampling interval. Clamped `[250, 60,000]`. |
| `uptimeMs`, `rssBytes` | `qint64` | Uptime + resident set memory sample. |

### 2.6 Capture-cost control domain

| Property | Type | Semantics |
|---|---|---|
| `captureProfile` | `int(enum)` | Capture preset: `FullCapture(0)`, `BalancedCapture(1)`, `LowLatencyCapture(2)`. |
| `pointerHitTestingEnabled` | `bool` | Enables/disables pointer UI hit-test work. |
| `pointerHitTestMinIntervalMs` | `int` | Minimum interval for pointer hit-test recomputation. Clamped `[0,1000]`. |
| `uiTrackingEnabled` | `bool` | Enables/disables UI lifecycle tree tracking and related events. |
| `recentEventCapacity` | `int` | Ring-buffer capacity. Clamped `[16, 4096]`. |
| `recentEventCount` | `int` | Current ring-buffer size. |

## 3. Capture Profile Presets

`captureProfile` applies a bundle of low-level policy toggles.

| Profile | High-frequency min interval | Runtime-state signal interval | Pointer hit-test | UI tracking |
|---|---:|---:|---|---|
| `FullCapture` | 16ms | 16ms | enabled | enabled |
| `BalancedCapture` | 24ms | 24ms | enabled (`pointerHitTestMinIntervalMs=16`) | enabled |
| `LowLatencyCapture` | 33ms | 33ms | disabled | disabled |

Notes:

- Presets are intended for runtime switching (debug build vs production interactive mode).
- Explicit property writes after preset selection are allowed and can fine-tune behavior.

## 4. Core Method Contract

### 4.1 Lifecycle

#### `start()`

- Installs global app event filter.
- Starts idle and OS timers.
- Emits `daemonStateChanged`, `runningChanged`.
- Records `daemon-started` event.

#### `stop()`

- Removes event filter.
- Clears bound window and tracked UI tree.
- Stops timers.
- Emits `daemonStateChanged`, `runningChanged`.
- Records `daemon-stopped` event.

#### `attachWindow(window)`

- Accepts `QObject*`; only `QQuickWindow*` is valid.
- Ensures daemon started.
- Replaces prior attached window binding.
- Records `window-attached` event.
- If `uiTrackingEnabled=true`, recursively tracks UI tree.
- Refreshes pointer-ui snapshot.
- On window destruction, records `window-detached` and clears attachments.

### 4.2 Runtime state and counters

#### `markActivity()`

- Updates activity timestamps.
- Exits idle state if currently idle.

#### `resetCounters()`

- Resets keyboard/mouse/UI counters and last-state fields.
- Clears high-frequency last-record bookkeeping.
- Preserves daemon run state (does not stop daemon).
- Records `counters-reset` event.

### 4.3 Snapshot and query APIs

#### `snapshot(): map`

Broad runtime summary:

- running + counters + idle + pid + rss + uptime + sequence + last event + input state.

#### `daemonHealth(): map`

Health-focused summary:

- running + attachedWindow + boot epoch + sequence + ring status + idle + pid + last event + input.

#### `inputState(): map`

Input-centric state payload:

- pointer global coordinates, mouse button flags, key states, modifier union, pointerUi, press/release timing.
- Touch input is normalized onto the same primary-pointer contract as desktop mouse input:
  the active touch contact is reported as `Qt::LeftButton`, updates contribute to mouse counters/signals,
  and release/cancel clears the button state.

#### `recentEvents(): list` / `clearRecentEvents()`

- ring-buffer read and reset operations.

#### `hitTestUiAt(globalX, globalY): map`

- On success returns detailed hit info.
- If hit-test fails or unavailable, returns fallback map with `"unknown"` metadata.

## 5. High-Frequency Event Behavior

High-frequency classes:

- `mouse-move`
- `hover-move`

Two-stage control:

1. **Payload construction sampling**  
   `shouldSampleHighFrequencyPayload()` decides whether to build rich payload/hit-test data.
2. **Record emission throttling**  
   `shouldSkipHighFrequencyRecord()` decides whether to emit/store event.

Implication:

- In throttled windows, expensive payload work is skipped early.
- Event sequence and ring-buffer volume are bounded by the same interval policy.

## 6. Pointer Hit-Test Modes and Costs

### Mode matrix

| Configuration | Behavior | Cost |
|---|---|---|
| `pointerHitTestingEnabled=true`, interval `0` | recompute on each refresh request | highest |
| `pointerHitTestingEnabled=true`, interval `>0` | recompute at most once per interval | medium |
| `pointerHitTestingEnabled=false` | always fallback pointer map | lowest |

### Fallback contract

When hit-test is disabled/unavailable, fallback contains:

- `globalX`, `globalY`
- `insideWindow=false`
- `objectName="unknown"`, `className="unknown"`, `path="unknown"`

This guarantees schema stability for consumers that require fields to exist.

## 7. UI Tracking Behavior

When `uiTrackingEnabled=true`:

- child additions are recursively tracked under attached window tree,
- show/hide/create/destroy transitions update counters and emit `uiEvent`,
- `ui-event` entries are recorded into runtime event stream.

When `uiTrackingEnabled=false`:

- tracked tree is detached,
- UI lifecycle counters stop increasing from new lifecycle transitions,
- input/pointer state APIs remain operational.

## 8. Integration Patterns

### 8.1 Low-latency production profile

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.RuntimeEvents.captureProfile = LV.RuntimeEvents.LowLatencyCapture
    LV.RuntimeEvents.start()
    LV.RuntimeEvents.attachWindow(rootWindow)
}
```

### 8.2 Balanced interactive diagnostics

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.RuntimeEvents.captureProfile = LV.RuntimeEvents.BalancedCapture
    LV.RuntimeEvents.pointerHitTestMinIntervalMs = 24
    LV.RuntimeEvents.start()
    LV.RuntimeEvents.attachWindow(rootWindow)
}
```

### 8.3 Full diagnostic capture (short sessions)

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.RuntimeEvents.captureProfile = LV.RuntimeEvents.FullCapture
    LV.RuntimeEvents.recentEventCapacity = 1024
}
```

## 9. Failure/Troubleshooting Matrix

| Symptom | Likely Cause | Verification | Action |
|---|---|---|---|
| `running=false` unexpectedly | daemon not started or stopped | inspect startup order | call `start()` before consumers subscribe |
| `pointerUi` lacks meaningful hit info | no attached window or hit-test disabled | check `daemonHealth().attachedWindow`, `pointerHitTestingEnabled` | attach window, enable hit-test if needed |
| high CPU during pointer move | full capture + per-event hit-test | inspect `captureProfile` | move to `Balanced` or `LowLatency` |
| UI counters remain zero | ui tracking disabled | check `uiTrackingEnabled` | enable tracking if lifecycle telemetry is required |
| event list grows too fast | capacity too large + full capture | check `recentEventCapacity` and profile | reduce capacity or profile level |
| idle never entered | frequent `markActivity` or low timeout mismatch | inspect `idleTimeoutMs`, external activity calls | tune timeout and activity call sites |

## 10. Data Contract Notes for Consumers

- `eventRecorded(eventData)` entries include:
  - `sequence`,
  - `type`,
  - `timestampEpochMs`,
  - `uptimeMs`,
  - optional `payload`.
- Consumers must not assume payload exists for every event type.
- Consumers must treat unknown/new event types as forward-compatible inputs.

## 11. Codex-Oriented Playbook

This section is for Codex workflows that modify runtime/event pipelines.

### 11.1 Profile-first patch strategy

When optimizing latency-sensitive demos or features:

1. switch to `captureProfile` first,
2. tune `pointerHitTestMinIntervalMs` second,
3. adjust `recentEventCapacity` last.

This preserves functionality while reducing capture cost with minimal semantic drift.

### 11.2 Patch safety rules for Codex

- Keep `eventRecorded` schema backward-compatible (`sequence`, `type`, timestamp keys).
- Preserve fallback `pointerUi` shape when disabling hit-testing.
- Treat high-frequency throttling as a performance policy, not a data-loss bug by default.
- Keep lifecycle idempotence (`start()`, `stop()`, repeated `attachWindow()` calls).

### 11.3 Regression checklist for Codex

After runtime-event edits, validate:

1. counters still increment for at least one keyboard/mouse path,
2. `daemonHealth().running` and `attachedWindow` remain truthful,
3. low-latency profile disables pointer/UI tracking as expected,
4. ring buffer remains bounded by capacity.

## 12. Validation Checklist

- `start()` + `attachWindow()` called for target window.
- `captureProfile` selected intentionally for deployment mode.
- `recentEventCapacity` sized for expected burst horizon.
- pointer/UI tracking toggles match feature requirements.
- high-frequency noise is throttled without losing required business signals.

## 13. Related APIs

- `Backend`: mirrors runtime events into backend-owned cache.
- `EventListener` (QML): consumes runtime/global events for interaction handling.
- `ApplicationWindow`: typical host that wires runtime monitoring on startup.
