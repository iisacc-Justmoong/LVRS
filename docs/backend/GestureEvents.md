# GestureEvents

Location: `backend/runtime/gestureevents.h` / `backend/runtime/gestureevents.cpp`

`GestureEvents` is the LVRS high-level gesture recognition singleton.  
It sits above `RuntimeEvents` and turns raw touch / native gesture runtime records into QML-friendly gesture events.

## 1. Scope and Responsibilities

`GestureEvents` owns:

- touch session tracking on top of `RuntimeEvents::eventRecorded`,
- high-level gesture classification for `touchStarted`, `touchUpdated`, `touchEnded`, `touchCancelled`,
- derived semantic events for `pressStarted`, `pressEnded`, `scrollStarted`, `scrollUpdated`, `scrollEnded`,
  `holdStarted`, `dragStarted`, `dragUpdated`, `dragEnded`, `swipeDetected`,
- native gesture passthrough normalization via `nativeGestureDetected`,
- stable last-event inspection via `gestureSequence` and `lastGesture`.

`GestureEvents` does **not** own:

- raw application event filtering (`RuntimeEvents` owns this),
- recent-event buffering / daemon health telemetry (`RuntimeEvents` owns this),
- backend mirroring / persistence (`Backend` owns this),
- multi-contact gesture taxonomy beyond the primary contact path.

## 2. Property API

| Property | Default | Meaning |
|---|---:|---|
| `runtimeAttached` | `false` | True when a `RuntimeEvents` source is bound. |
| `holdThresholdMs` | `450` | Minimum stationary press duration required for `holdStarted`. |
| `dragThresholdPx` | `12.0` | Travel distance required before drag classification starts. |
| `scrollThresholdPx` | `12.0` | Dominant-axis one-finger travel distance required before scroll classification starts. |
| `swipeThresholdPx` | `48.0` | Minimum total travel distance required for swipe detection. |
| `swipeMaxDurationMs` | `700` | Swipe must finish within this duration. |
| `axisDominanceRatio` | `1.35` | Axis ratio used to classify dominant direction (`x`, `y`, `diagonal`). |
| `gestureSequence` | `0` | Monotonic sequence for recognized gesture payloads. |
| `lastGesture` | `{}` | Last published high-level gesture payload. |

## 3. Method Contract

### `attachRuntime(runtimeObject = null): bool`

- Accepts an explicit `RuntimeEvents` object or resolves the singleton automatically.
- Connects to `RuntimeEvents::eventRecorded`.
- Returns `true` when binding succeeded.

### `detachRuntime()`

- Disconnects from the bound runtime source.
- Clears active touch-session state.

### `resetState()`

- Clears active touch/drag/hold session state.
- Resets in-progress timers.
- Preserves runtime binding.

## 4. Signals

- `gestureRecognized(eventData)`
- `touchStarted(eventData)`
- `touchUpdated(eventData)`
- `touchEnded(eventData)`
- `touchCancelled(eventData)`
- `pressStarted(eventData)`
- `pressEnded(eventData)`
- `holdStarted(eventData)`
- `dragStarted(eventData)`
- `dragUpdated(eventData)`
- `dragEnded(eventData)`
- `scrollStarted(eventData)`
- `scrollUpdated(eventData)`
- `scrollEnded(eventData)`
- `swipeDetected(eventData)`
- `nativeGestureDetected(eventData)`

`gestureRecognized` is emitted for every published high-level payload, including the more specific signals above.

## 5. Payload Schema

### 5.1 Common gesture fields

Every published payload includes:

- `sequence`
- `gestureType`
- `interactionKind`
- `classification`
- `source`
- `timestampEpochMs`
- `x`, `y`, `globalX`, `globalY`
- `ui`

`ui` and `originUi` use the enriched `RuntimeEvents.hitTestUiAt()` schema:

- logical target identity: `objectName`, `className`, `componentName`, `qmlId`, `qmlBaseUrl`, `path`
- layer and hierarchy: `layerKind`, `depth`, `hierarchy`, root/window fields
- raw deepest leaf: `hitObjectName`, `hitClassName`, `hitPath`, `hitComponentName`, `hitQmlId`, `hitLocalX`, `hitLocalY`, `hitDepth`

### 5.2 Touch-derived gesture fields

`touch*`, `press*`, `scroll*`, `holdStarted`, `drag*`, and `swipeDetected` additionally include:

- `sessionId`
- `previousX`, `previousY`
- `startX`, `startY`, `startGlobalX`, `startGlobalY`
- `deltaX`, `deltaY`
- `totalDeltaX`, `totalDeltaY`
- `absoluteDeltaX`, `absoluteDeltaY`
- `distance`
- `durationMs`
- `pressDurationMs`
- `directionX` (`positive|negative|none`)
- `directionY` (`positive|negative|none`)
- `dominantAxis` (`x|y|diagonal|none`)
- `holdActive`, `dragActive`, `scrollActive`
- `holdThresholdMs`, `dragThresholdPx`, `scrollThresholdPx`, `swipeThresholdPx`, `swipeMaxDurationMs`
- `phase`
- `pointCount`, `fingerCount`, `activeFingerCount`, `maximumFingerCount`
- `pressedFingerCount`, `updatedFingerCount`, `stationaryFingerCount`, `releasedFingerCount`
- `primaryPointId`, `multiTouch`, `released`, `cancelled`, `releaseEpochMs`
- `points`
- `buttons`
- `pressedMouseButtons`
- `modifiers`
- `mouseButtonPressed`
- `originUi`

Each entry in `points` mirrors native `QEventPoint` detail from `RuntimeEvents`, including state name,
timestamp/pressTimestamp/timeHeld, pressure/rotation/ellipse, velocity, and position families
(`position*`, `pressPosition*`, `lastPosition*`, `scene*`, `global*`).

### 5.3 Scroll-specific fields

`scrollStarted`, `scrollUpdated`, and `scrollEnded` add:

- `scrollAxis` (`x|y`)
- `scrollDirection`
- `scrollDeltaX`
- `scrollDeltaY`

### 5.4 Swipe-specific fields

`swipeDetected` adds:

- `swipeDirection`
- `velocityX`
- `velocityY`
- `speed`

`swipeDirection` is derived from total delta and can be:

- `leftToRight`
- `rightToLeft`
- `topToBottom`
- `bottomToTop`
- one of the four diagonal tokens

### 5.5 Native gesture fields

`nativeGestureDetected` adds:

- `nativeGestureType`
- `fingerCount`
- `value`
- `deltaX`
- `deltaY`
- `buttons`
- `pressedMouseButtons`
- `modifiers`

## 6. Recognition Rules

### Press

- Starts from `TouchBegin`.
- Emits `pressStarted` immediately with native finger-count and point metadata.
- Emits `pressEnded` on release/cancel with `pressDurationMs`, `releaseEpochMs`, `released`, `cancelled`, and `finalInteractionKind`.

### Scroll

- Starts after one active touch contact moves beyond `scrollThresholdPx` on a dominant `x` or `y` axis.
- Emits `scrollStarted` once, `scrollUpdated` on subsequent recognized movement, and `scrollEnded` on release/cancel.
- Scroll classification is continuous-motion oriented; a fast release can still also qualify for `swipeDetected`.

### Hold

- Starts from `TouchBegin`.
- Fires only when the pointer stays within `dragThresholdPx` until `holdThresholdMs` elapses.

### Drag

- Starts when total distance from the touch origin reaches `dragThresholdPx`.
- Emits `dragStarted` once, then `dragUpdated` on later updates, then `dragEnded` on release/cancel.

### Swipe

- Evaluated on touch end.
- Requires `distance >= swipeThresholdPx`.
- Requires `durationMs <= swipeMaxDurationMs`.
- Direction is derived from `axisDominanceRatio`.

### Native gesture

- Uses the raw `native-gesture` records already captured by `RuntimeEvents`.
- Preserves the platform gesture kind, `fingerCount`, scalar `value`, and `deltaX/Y` where Qt reports them.

## 7. Integration Patterns

### 7.1 Direct singleton usage

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.RuntimeEvents.start()
    LV.RuntimeEvents.attachWindow(rootWindow)
    LV.GestureEvents.attachRuntime(LV.RuntimeEvents)
}

Connections {
    target: LV.GestureEvents
    function onSwipeDetected(eventData) {
        console.log(eventData.swipeDirection, eventData.totalDeltaX, eventData.totalDeltaY)
    }
}
```

### 7.2 Preferred QML consumption through `EventListener`

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "swipeDetected"
    action: function(eventData) {
        console.log(eventData.swipeDirection)
    }
}
```

`EventListener` auto-attaches `RuntimeEvents` and `GestureEvents` when a gesture trigger is used.

## 8. Current Recognition Boundary

- Recognition is primary-contact centric.
- Multi-touch point arrays, finger counts, and maximum session finger count are forwarded. Directional scroll/drag/swipe
  classification still tracks the primary contact path unless the platform emits a native gesture.
- Raw desktop mouse semantics remain handled by `EventListener` pointer/global triggers, not by `GestureEvents`.

## 9. Troubleshooting Matrix

| Symptom | Likely Cause | Verification | Action |
|---|---|---|---|
| no gesture callbacks | runtime not attached | inspect `runtimeAttached` | call `attachRuntime()` or use `EventListener` gesture triggers |
| hold never fires | drag threshold reached too early | inspect `distance` vs `dragThresholdPx` | increase `dragThresholdPx` or reduce incidental movement |
| swipe not detected | duration too long or distance too short | inspect `durationMs`, `distance` | tune `swipeThresholdPx` / `swipeMaxDurationMs` |
| direction looks diagonal | axis dominance ratio too strict | inspect `dominantAxis` | lower `axisDominanceRatio` |
| need raw full gesture history | using only `lastGesture` | inspect `gestureSequence` changes | subscribe to `gestureRecognized` and keep your own buffer |

## 10. Related APIs

- `RuntimeEvents`: raw event capture source.
- `EventListener`: QML trigger bridge for gesture callbacks.
- `Backend`: backend mirror for raw runtime events.
