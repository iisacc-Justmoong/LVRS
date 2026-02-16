# Backend

Location: `backend/io/backend.h` / `backend/io/backend.cpp`

`Backend` is the QML-facing bridge singleton for filesystem helpers and runtime event hook caching.

## Purpose

- File I/O helper APIs for QML.
- Runtime event hook lifecycle management.
- Bounded mirror cache of user/runtime events.
- Stable input-state relay for consumers requiring backend-first reads.

## Properties

- `lastError: string`  
  Last error produced by Backend operation.
- `userEventHooked: bool`  
  Whether Backend is currently connected to runtime event stream.
- `hookedEventCount: int`  
  Number of cached hooked events.
- `hookedEventCapacity: int`  
  Event cache limit. Writes are clamped to `[64, 32768]`.
- `lastHookedEvent: map`  
  Latest event entry mirrored into backend cache.
- `lastHookedInputState: map`  
  Latest known input snapshot from payload/runtime fallback.

## Methods

File helpers:

- `saveTextFile(path, text): bool`
- `readTextFile(path): string`
- `ensureDir(path): bool`
- `writableLocation(location): string`

Runtime hook lifecycle:

- `hookUserEvents(): bool`
- `unhookUserEvents(): void`
- `clearHookedUserEvents(): void`

Cache/snapshot APIs:

- `hookedUserEvents(limit = -1): list`
- `hookedUserEventSummary(): map`
- `currentUserInputState(): map`

## Hook Lifecycle

1. Resolve `RuntimeEvents` instance.
2. Call `RuntimeEvents.start()`.
3. Disconnect previous connections (if any).
4. Subscribe to `eventRecorded` and runtime destruction.
5. Ingest current runtime `recentEvents()` into backend cache.
6. Mark `userEventHooked = true`.

## Cache Semantics

- Every cached event gets `hookEpochMs` at mirror time.
- Per-type counters are maintained by event `type`.
- Capacity overflow evicts oldest entries and decrements counters accordingly.

## Error Semantics

`lastError` is reset on operation start and set when operation fails.
Common failures:

- empty path/key arguments,
- file open/commit failure,
- runtime singleton unavailable.

## Usage Example

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    if (!LV.Backend.hookUserEvents())
        console.warn("backend hook failed", LV.Backend.lastError)
}

function exportSnapshot(path) {
    const payload = JSON.stringify(LV.Backend.hookedUserEventSummary(), null, 2)
    if (!LV.Backend.saveTextFile(path, payload))
        console.warn("save failed", LV.Backend.lastError)
}
```

## Extended Usage: Event Snapshot Export

```qml
import LVRS 1.0 as LV

function exportRecent(path) {
    const events = LV.Backend.hookedUserEvents(256)
    const ok = LV.Backend.saveTextFile(path, JSON.stringify(events, null, 2))
    if (!ok)
        console.warn("export failed:", LV.Backend.lastError)
}
```

## Operational Guardrails

- Keep `hookedEventCapacity` sized by expected burst volume.
- Call `clearHookedUserEvents()` when entering a new measurement phase.
- Do not assume `currentUserInputState()` is non-empty before runtime hook/bootstrap is complete.

## Failure Checklist

If `hookUserEvents()` fails:

1. ensure runtime singleton exists in app process,
2. ensure QML module was loaded before hook call,
3. inspect `lastError` for exact failure reason.

## FAQ

Q. Why is `lastHookedInputState` stale after unhook?  
A. It reflects last captured state snapshot. Re-hook runtime stream for live updates.
