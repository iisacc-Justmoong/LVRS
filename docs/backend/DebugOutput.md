# Debug Output Schema

Location: `backend/runtime/debuglogger.h`, `backend/runtime/debuglogger.cpp`, `backend/runtime/runtimeevents.h`, `backend/runtime/runtimeevents.cpp`, `backend/io/backend.h`, `backend/io/backend.cpp`, `example/VisualCatalog/qml/Main.qml`, `qml/components/control/util/EventListener.qml`

This document defines the schema used by debug and runtime-event data that is emitted and displayed in the LVRS demo app. It is intended to let developers trace relationships between stdout logs, runtime events, and QML monitor views without ambiguity.

## 1. Output Channels

- Bootstrap stdout diagnostics: emitted by `preApplicationBootstrap`, `postApplicationBootstrap`, and `runBootstrappedQmlApp` before `LV.Debug` is necessarily available.
- Stdout text log: emitted by `LV.Debug` through `printEntryToStdout()`.
- Stdout JSON log: when `LV.Debug.jsonOutput == true`, an additional line is emitted as `[DEBUG-ENTRY] { ... }`.
- Debug memory buffer: available via `LV.Debug.entries()/filteredEntries()/summary()`.
- RuntimeEvents buffer: available via `LV.RuntimeEvents.recentEvents()`.
- Backend hook buffer: available via `LV.Backend.hookedUserEvents()`.
- Demo Event Listener Monitor: displayed by `runtimeConsoleRows` and `eventMonitorSamplesModel` in `example/VisualCatalog/qml/Main.qml`.

## 2. Current Default Output Policy (Main.qml Bootstrap)

The default values configured by `Component.onCompleted` and `debuggerBootstrap()` are:

- `LV.Debug.enabled = true` (stdout output enabled)
- `LV.Debug.verboseOutput = false`
- `LV.Debug.jsonOutput = false`
- `LV.Debug.runtimeEchoEnabled = false`
- `LV.Debug.runtimeEchoMinIntervalMs >= 250`
- `LV.Debug.stdoutMinimumLevel = "WARN"`
- `LV.Debug.stdoutNoiseReductionEnabled = true`
- `LV.Debug.runtimeCaptureEnabled = true`

The effective behavior is warning-centric (`WARN/ERROR`) output with high-frequency input/lifecycle noise suppression.

Bootstrap diagnostics are separate from `LV.Debug`. They are always plain stdout lines prefixed with `LVRS bootstrap.` and carry compact JSON payloads describing bootstrap options, render defaults, backend probing, import-path resolution, and font-policy decisions.

### Bootstrap stdout line format

- Prefix: `LVRS bootstrap.<stage>`
- Payload: compact JSON map
- Level: `INFO` for normal progress, `WARN` for fallback/soft degradation, `CRITICAL` for fatal bootstrap failure

Common bootstrap stages:

- `pre.options`
- `pre.render-quality`
- `pre.quick-style`
- `graphics.probe`
- `graphics.selected`
- `graphics.fallback`
- `pre.complete`
- `post.application`
- `post.font-policy`
- `entry.import-paths`
- `entry.load-request`

## 3. LV.Debug Entry Schema

`LV.Debug.log/warn/error` and `RuntimeEvents.eventRecorded` are stored in the same entry buffer (`m_entries`).

### Common Fields

- `source`: `"logger"` or `"runtime"`
- `level`: `"LOG" | "WARN" | "ERROR" | "RUNTIME"`
- `component`: e.g. `"Main"`, `"RenderMonitor"`, `"RuntimeEvents"`
- `event`: event name
- `message`: default UI/stdout line message
- `timestampEpochMs`: epoch ms
- `timestamp`: `HH:MM:SS.CS` (centiseconds)
- `timestampIso`: UTC ISO string
- `sequence`: DebugLogger-local sequence (increments from 1)
- `sessionElapsedMs`: elapsed ms since debug-session start
- `processId`
- `threadId`
- `applicationName`
- `applicationVersion`
- `data` (optional)
- `runtimeEventSequence` (when RuntimeEvents is attached)

### Additional Fields for `runtime` Source

- `uptimeMs`
- `rawEvent` (raw `RuntimeEvents` event)

## 4. Stdout Output Rules

### Level Priority

- `ERROR=3`, `WARN=2`, `RUNTIME=1`, `LOG=1`, `NONE=99`
- Output is emitted only if `shouldOutputLevel(entry.level) >= stdoutMinimumLevel`.

### Noise Filter (`stdoutNoiseReductionEnabled=true`)

- Blocks `created`, `shown`, `hidden`, `destroyed` at `LOG` level
- Blocks `RenderMonitor.render-stats` at `LOG` level
- Blocks high-frequency `RUNTIME` events:
  - `ui-event`, `mouse-move`, `hover-move`, `mouse-wheel`, `mouse-press`, `mouse-release`, `mouse-double-click`
  - `key-press`, `key-release`, `touch-event`, `tablet-event`, `tablet-proximity`, `native-gesture`

### Line Format

- `verboseOutput=false`: prints only `entry.message`
- `verboseOutput=true`: `[timestamp] [level] #sequence component.event pid=... tid=... runtimeSeq=... data=...`
- `jsonOutput=true`: appends `[DEBUG-ENTRY] {compact-json}` after the line above

## 5. RuntimeEvents Raw Event (`eventRecorded`) Schema

Common fields emitted by `recordRuntimeEvent()`:

- `sequence`
- `type`
- `timestampEpochMs`
- `uptimeMs`
- `payload` (optional)

Major `type` values and key `payload` fields:

- `key-press`, `key-release`: `key`, `keyName`, `modifiers`, `autoRepeat`, `text`, `pressedKeys`, `pressedKeyCodes`, `activeModifierNames`
- `mouse-move`, `hover-move`: `x`, `y`, `buttons`, `pressedMouseButtons`, `modifiers`, `mouseButtonPressed`, `pointerUi`, `pointerObjectName`, `pointerClassName`, `pointerPath`
- `mouse-press`, `mouse-release`, `mouse-double-click`: keys above + `button`, `lastMousePressEpochMs`/`lastMouseReleaseEpochMs`, elapsed fields
- `mouse-wheel`: keys above + `angleDeltaX/Y`, `pixelDeltaX/Y`, `phase`, `inverted`
- `touch-event`: `phase`, `pointCount`, `x`, `y`, `buttons`, `pressedMouseButtons`, `mouseButtonPressed`,
  `lastMousePressEpochMs`, `lastMouseReleaseEpochMs`, elapsed timing fields, `points[]`, `pointerUi`,
  `pointerObjectName`, `pointerClassName`, `pointerPath`
- `tablet-event`: `phase`, `pressure`, `rotation`, `xTilt`, `yTilt`, `pointerType`, etc.
- `tablet-proximity`: `phase`
- `native-gesture`: `gestureType`, `value`, pointer information
- `context-requested`: `x`, `y`, `modifiers`, `buttons`, `reason`, `pointerUi`
- `ui-event`: `eventType`, `objectName`, `className`, `visible`
- `daemon-started`, `daemon-stopped`, `window-attached`, `window-detached`, `counters-reset`

## 6. Runtime State Snapshot API Output

### `LV.RuntimeEvents.snapshot()`

- `running`
- key/mouse/UI counters
- `idle`, `idleForMs`
- `pid`, `rssBytes`, `uptimeMs`
- `daemonBootEpochMs`, `eventSequence`, `recentEventCount`
- `lastEvent`
- `input` (result of `inputState()`)

### `LV.RuntimeEvents.daemonHealth()`

- `running`, `attachedWindow`
- `bootEpochMs`, `eventSequence`
- `recentEventCount`, `recentEventCapacity`
- `idle`, `idleForMs`
- `pid`
- `lastEvent`
- `input`

### `LV.RuntimeEvents.inputState()`

- pointer coordinates/buttons/button names/pressed state
- last press/release timestamps and elapsed durations
- `activePressDurationMs`
- `pointerUi` (hit-test result)
- `anyKeyPressed`, `pressedKeys`, `pressedKeyCodes`
- `activeModifiers`, `activeModifierNames`
- `lastKey`, `lastKeyText`, `lastKeyModifiers`

## 7. Backend Hook Output

`LV.Backend.hookUserEvents()` subscribes to RuntimeEvents and keeps a separate buffer.

### `LV.Backend.hookedUserEvents(limit)`

- list of RuntimeEvents raw events plus `hookEpochMs`

### `LV.Backend.hookedUserEventSummary()`

- `hooked`
- `eventCount`
- `capacity`
- `lastEvent`
- `input` (result of `currentUserInputState()`)
- `typeCounts` (aggregation by event type)
- `runtimeEventSequence` (when runtime is attached)

## 8. Main.qml Event Listener Monitor Output

### Runtime Console Row (`runtimeConsoleRows[]`) Schema

- `category`: `runtime|input|ui|render|navigation|system`
- `source`
- `type`
- `sequence`
- `timestampEpochMs`
- `uptimeMs`
- `payload`
- `summary` (type-specific summary string)
- `detail` (abbreviated payload string)

Display timestamps follow `runtimeConsoleTimestamp()` in `HH:MM:SS.mmm` format.

Category mapping follows `runtimeConsoleCategoryForType()`:

- Input family: `key-*`, `mouse-*`, `touch-*`, `tablet-*`, `native-gesture*`, `hover-*`, `context-*`, `global-*` -> `input`
- `ui-event` -> `ui`
- `render-*` -> `render`
- `route-*`, `viewstack-*` -> `navigation`
- `daemon-*`, `window-*`, `catalog-*`, `counters-*` -> `runtime`
- Others -> `system`

### EventListener Sample (`eventMonitorSamplesModel`) Schema

- `trigger`
- `source`
- `timestampEpochMs`
- `payload`

If samples exceed `eventMonitorMaxSamples`, oldest entries are removed first (FIFO).

## 9. EventListener Callback Payload Schema

Based on `qml/components/control/util/EventListener.qml`:

- Local pointer triggers (`clicked|pressed|released`):
  - `x`, `y`, `globalX`, `globalY`, `button`, `buttons`, `modifiers`, `isGlobal=false`
  - `ui` (optional, `includeUiHit=true`)
  - `input` (optional, `includeInputState=true`)
  - `backend` (optional, `includeBackendSummary=true`)
- Global triggers (`globalPressed|globalContextRequested`):
  - `x`, `y`, `globalX`, `globalY`, `buttons`, `modifiers`, `isGlobal=true`
  - `ui` (optional, `includeUiHit=true`)
  - `input` (optional, `includeInputState=true`)
  - `backend` (optional)
  - for context events, `reason` and `source(mouse|context)` are added
- Key/wheel triggers pass Qt event objects directly.

Deduplication:

- Global-press deduplication: `globalPressDedupMs` (default 24ms), coordinate tolerance `globalPressDedupTolerancePx` (default 2px)
- Context deduplication: `contextDedupMs` (default 180ms), coordinate tolerance `contextDedupTolerancePx` (default 2px)

## 10. Render-Performance Alert Output Conditions

`evaluateRenderPerformance()` runs periodically and emits through `LV.Debug`.

- Severe: `lastFrameMs >= 50` or `fps < 18` -> `ERROR render-performance-severe` (minimum interval: 1600ms)
- Degraded: `lastFrameMs >= 33` or `fps < 30` -> `WARN render-performance-degraded` (minimum interval: 2000ms)
- Recovery: `WARN render-performance-recovered` after normal state is maintained for 3 seconds

Common payload:

- `fps`
- `lastFrameMs`
- `frameCount`
- `backend` (`LV.RenderQuality.graphicsBackend`)

## Consumer Implementation Notes

When integrating with log viewers or telemetry pipelines:

- treat `sequence` as ordered-event cursor,
- treat `timestamp` fields as wall-clock and compute latency externally,
- avoid strict schema parsing on optional payload fields for forward compatibility.

## Parser Safety Recommendations

- always null-check nested maps (`payload`, `input`, `ui`),
- tolerate missing keys for older entries,
- keep unknown field passthrough to preserve future extensibility.
