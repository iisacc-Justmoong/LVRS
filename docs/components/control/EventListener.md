# EventListener

Location: `qml/components/control/util/EventListener.qml`

`EventListener` is the LVRS interaction bridge component that maps trigger tokens to callback events.  
Current policy is incident-centric: it avoids continuous state harvesting unless explicitly enabled.

## 1. Design Goal

- Deliver event callbacks for actual incidents (`click`, `press`, `release`, `context request`, etc.).
- Avoid high-cost payload enrichment by default.
- Prevent chain reactions caused by unnecessary runtime snapshot/hit-test pulls.

## 2. Supported Triggers

- Pointer-local: `clicked`, `pressed`, `released`, `entered`, `exited`, `hoverChanged`
- Wheel: `wheel`
- Keyboard: `keyPressed`, `keyReleased`
- Global runtime: `globalPressed`, `globalContextRequested`
- Gesture runtime: `touchStarted`, `touchUpdated`, `touchEnded`, `touchCancelled`, `pressStarted`, `pressEnded`, `holdStarted`, `longPressed`, `dragStarted`, `dragUpdated`, `dragEnded`, `scrollStarted`, `scrollUpdated`, `scrollEnded`, `swipeDetected`, `nativeGestureDetected`, `gestureRecognized`

## 3. Core API

| Property | Default | Meaning |
|---|---|---|
| `trigger` | `"clicked"` | Event source selector. |
| `action` | `null` | Callback invoked when trigger fires. |
| `enabled` | `true` | Listener enable flag. |
| `acceptedButtons` | `Qt.LeftButton` | Mouse buttons accepted by local pointer path. |
| `macControlClickAsRight` | `true` | Treat macOS Ctrl+Left as context gesture. |
| `includeUiHit` | `false` | Enrich payload with `ui` hit-test info (opt-in). |
| `includeInputState` | `false` | Enrich payload with runtime/backend `input` snapshot (opt-in). |
| `preferBackendState` | `false` | When `includeInputState=true`, prefer backend-first input state resolution. |
| `includeBackendSummary` | `false` | Include backend hook summary map in payload (opt-in). |
| `globalPressDedupMs` | `24` | Global press dedup time window. |
| `globalPressDedupTolerancePx` | `2.0` | Global press dedup spatial tolerance. |
| `contextDedupMs` | `180` | Context dedup time window. |
| `contextDedupTolerancePx` | `2.0` | Context dedup spatial tolerance. |

## 4. Incident-Centric Payload Rules

### 4.1 Default mode (recommended)

By default:

- no `input` snapshot is fetched,
- no UI hit-test traversal is executed,
- callback receives only direct incident fields.

This minimizes runtime coupling and avoids invisible view-tree tracking for non-required cases.

### 4.2 Enriched mode (explicit opt-in)

Set `includeInputState: true` when callback logic truly needs runtime state coherence.  
Set `includeUiHit: true` only when UI hit metadata is required for decisions (for example, contextual menus).

### 4.3 Backend summary mode

`includeBackendSummary: true` is diagnostic-oriented and can be expensive in high-rate flows.  
Use only for tooling/debug dashboards, not hot-path interaction callbacks.

## 5. Payload Schema

### Local pointer (`clicked|pressed|released`)

Always:

- `x`, `y`, `globalX`, `globalY`
- `button`, `buttons`, `modifiers`
- `isGlobal=false`

Optional:

- `ui` when `includeUiHit=true`
- `input` when `includeInputState=true`
- `backend` when `includeBackendSummary=true`

### Global pointer (`globalPressed|globalContextRequested`)

Always:

- `x`, `y`, `globalX`, `globalY`
- `buttons`, `modifiers`
- `isGlobal=true`

Context-only additions:

- `reason`
- `source` (`"mouse"` or `"context"`)

Optional enrichments follow same toggles as local pointer payloads.

### Wheel / key triggers

- wheel/key triggers forward Qt event objects directly.

### `hoverChanged`

- payload: `{ containsMouse: bool }`

### Gesture triggers

Gesture triggers receive the normalized payload published by `GestureEvents`.

Always:

- `sequence`, `gestureType`, `interactionKind`, `source`
- `classification`
- `timestampEpochMs`
- `x`, `y`, `globalX`, `globalY`

Touch-derived gesture triggers additionally receive:

- `sessionId`
- `previousX`, `previousY`
- `startX`, `startY`, `startGlobalX`, `startGlobalY`
- `deltaX`, `deltaY`
- `totalDeltaX`, `totalDeltaY`
- `absoluteDeltaX`, `absoluteDeltaY`
- `distance`, `durationMs`
- `pressDurationMs`
- `directionX`, `directionY`, `dominantAxis`
- `holdActive`, `dragActive`, `scrollActive`
- `phase`, `pointCount`, `fingerCount`, `activeFingerCount`, `maximumFingerCount`, `points`
- `pressedFingerCount`, `updatedFingerCount`, `stationaryFingerCount`, `releasedFingerCount`
- `primaryPointId`, `multiTouch`, `released`, `cancelled`, `releaseEpochMs`
- `buttons`, `pressedMouseButtons`, `modifiers`, `mouseButtonPressed`
- `ui`, `originUi`

`ui` / `originUi` include component-identification metadata:

- logical target: `objectName`, `className`, `componentName`, `qmlId`, `qmlBaseUrl`, `path`
- hierarchy: `layerKind`, `depth`, `hierarchy`
- raw deepest item: `hitObjectName`, `hitClassName`, `hitPath`, `hitComponentName`, `hitQmlId`

Swipe-specific additions:

- `swipeDirection`
- `velocityX`
- `velocityY`
- `speed`

Scroll-specific additions:

- `scrollAxis`
- `scrollDirection`
- `scrollDeltaX`
- `scrollDeltaY`

Native-gesture additions:

- `nativeGestureType`
- `fingerCount`
- `value`
- `deltaX`, `deltaY`

## 6. Dedup Behavior

- Global press dedup suppresses duplicate press incidents in a short time-distance window.
- Context dedup suppresses duplicate context incidents in its own window.
- Dedup affects callback emission only; it does not mutate external runtime daemon counters.

## 7. Usage Patterns

### 7.1 Minimal incident listener (preferred)

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "globalPressed"
    action: function(eventData) {
        // Use only coordinates/buttons for outside-dismiss logic.
    }
}
```

### 7.2 Context listener with UI hit metadata

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "globalContextRequested"
    includeUiHit: true
    action: function(eventData) {
        console.log(eventData.ui ? eventData.ui.path : "unknown")
    }
}
```

### 7.3 Explicit input-state opt-in

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "globalContextRequested"
    includeInputState: true
    preferBackendState: true
    action: function(eventData) {
        const input = eventData.input || ({})
        console.log(input.activeModifierNames)
    }
}
```

### 7.4 Gesture listener

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "swipeDetected"
    action: function(eventData) {
        console.log(eventData.swipeDirection, eventData.totalDeltaX, eventData.totalDeltaY)
    }
}
```

### 7.5 Mobile press/scroll listeners

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "pressEnded"
    action: function(eventData) {
        console.log(eventData.pressDurationMs, eventData.fingerCount, eventData.released)
    }
}

LV.EventListener {
    trigger: "scrollStarted"
    action: function(eventData) {
        console.log(eventData.scrollAxis, eventData.scrollDirection)
    }
}
```

## 8. Common Pitfalls

- Enabling `includeInputState` everywhere by habit.
- Enabling both `includeUiHit` and `includeBackendSummary` for high-frequency interaction paths.
- Using `hoverChanged` for business logic that should be press/release driven.
- Treating global triggers as local geometry-only events without global coordinate checks.
- Expecting `longPressed` to be a separate payload type; it is an alias of `holdStarted`.
- Expecting full multi-touch gesture taxonomy from `EventListener`; it forwards finger counts and native point arrays,
  while directional scroll/drag/swipe classification follows the primary contact unless Qt emits `nativeGestureDetected`.

## 9. Troubleshooting Matrix

| Symptom | Likely Cause | Verification | Action |
|---|---|---|---|
| callback not firing | wrong trigger token or disabled listener | inspect `trigger`, `enabled` | fix token/state |
| missing `ui` payload | `includeUiHit=false` | inspect listener props | enable only where needed |
| missing `input` payload | `includeInputState=false` | inspect listener props | enable explicitly for that listener |
| duplicate context callbacks | dedup window too loose for source | inspect dedup config | tune `contextDedup*` values |
| swipe/drag not firing | runtime not attached or thresholds too strict | inspect trigger type and movement payload | use gesture trigger path and tune thresholds through `GestureEvents` |
| heavy callback chain | unnecessary enrichment enabled globally | inspect props in hot path | reduce to minimal incident payload |

## 10. Codex-Oriented Playbook

### 10.1 Safe Codex defaults for new listeners

1. Keep `includeUiHit=false`.
2. Keep `includeInputState=false`.
3. Keep `includeBackendSummary=false`.
4. Enable enrichment only when callback logic requires it.

### 10.2 Codex anti-patterns

- Do not auto-inject input-state enrichment into every listener.
- Do not use global listeners for per-frame tracking semantics.
- Do not add side-effect-heavy callback bodies in shared root listeners.

### 10.3 Codex regression checklist

After edits:

1. global press/context listeners still fire exactly once per incident,
2. outside-dismiss flows still work with minimal payload,
3. gesture triggers auto-attach runtime and deliver recognized press/scroll/gesture payloads,
4. enriched listeners still receive requested optional fields when enabled.

## 11. Related APIs

- `RuntimeEvents`: runtime daemon source for global triggers.
- `GestureEvents`: high-level gesture source used by gesture triggers.
- `Backend`: optional backend-first input state source.
- `ApplicationWindow`: installs root-level global listeners for app-wide behavior.
