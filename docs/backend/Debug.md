# Debug

Location: `backend/runtime/debuglogger.h` / `backend/runtime/debuglogger.cpp`

`Debug` (`DebugLogger`) is the shared logging singleton for QML/C++ integration with memory buffer, filtering, and optional stdout echo.

## Purpose

- Centralize structured application/runtime logging.
- Provide bounded in-memory entry buffer for UI tools.
- Offer optional runtime stream capture from `RuntimeEvents`.

## Core Methods

Log emission:

- `log(component, event, data?)`
- `warn(component, event, data?)`
- `error(component, event, data?)`

Runtime attachment:

- `attachRuntimeEvents()`
- `detachRuntimeEvents()`

Query and maintenance:

- `entries(limit?)`
- `filteredEntries(limit?)`
- `summary()`
- `clearEntries()`
- `setFilters(levels, components, text)`

## Key Properties

Enablement:

- `enabled`
- `runtimeCaptureEnabled`
- `runtimeEchoEnabled`
- `paused`

Echo policy:

- `runtimeEchoMinIntervalMs`
- `runtimeEchoExcludeTypes`
- `stdoutMinimumLevel`
- `stdoutNoiseReductionEnabled`
- `verboseOutput`
- `jsonOutput`

Buffer/filter state:

- `maxEntries`, `entryCount`, `droppedCount`, `sequence`
- `runtimeAttached`
- `levelFilter`, `componentFilter`, `textFilter`
- `lastEntry`

## Runtime Capture Flow

1. `attachRuntimeEvents()` resolves runtime singleton.
2. Subscribe to `eventRecorded`.
3. Transform runtime event to debug entry.
4. Apply filters/output policy.
5. Append bounded entry buffer.

## Usage Example

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.Debug.enabled = true
    LV.Debug.attachRuntimeEvents()
    LV.Debug.setFilters(["WARN", "ERROR"], [], "")
}
```

## Related Schema

Detailed output entry schema and console row fields are defined in:

- `docs/backend/DebugOutput.md`

## Advanced Filter Example

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.Debug.enabled = true
    LV.Debug.runtimeCaptureEnabled = true
    LV.Debug.runtimeEchoEnabled = false
    LV.Debug.setFilters(["WARN", "ERROR"], ["RuntimeEvents", "PageRouter"], "")
}
```

## Performance Notes

- Keep `maxEntries` bounded for long sessions.
- Enable stdout JSON output only when downstream parsers require it.
- Use `runtimeEchoExcludeTypes` aggressively to suppress high-frequency noise.

## FAQ

Q. Does enabling logger change business logic behavior?  
A. It must not. Logger is observability-only and should never alter feature outcomes.
