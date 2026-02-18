# Backend

Location: `backend/io/backend.h` / `backend/io/backend.cpp`

`Backend` is the QML bridge singleton for filesystem utilities and mirrored runtime-event caching.  
It is intentionally simple: transport and bounded caching, not policy-heavy analytics.

## 1. Scope and Non-Scope

`Backend` owns:

- atomic text save/read helpers,
- async worker-pool based file/task dispatch for non-blocking UI flows,
- writable location lookup bridge,
- runtime-event hook lifecycle to `RuntimeEvents`,
- bounded mirrored event cache with per-type counters,
- last-known input-state relay.

`Backend` does **not** own:

- global event capture itself (`RuntimeEvents` owns this),
- file format parsing semantics beyond text read/write,
- long-term persistence/history storage.

## 2. Property API Contract

| Property | Type | Semantics | Edge/Clamp |
|---|---|---|---|
| `lastError` | `string` | Last operation error message. Cleared at operation start. | Empty string means no current error. |
| `userEventHooked` | `bool` | True when connected to runtime event stream. | False when runtime destroyed/unhooked. |
| `hookedEventCount` | `int` | Current mirrored event count. | Bounded by `hookedEventCapacity`. |
| `hookedEventCapacity` | `int` | Max mirrored events to retain. | Clamped `[64, 32768]`. |
| `lastHookedEvent` | `map` | Most recent mirrored event (with `hookEpochMs`). | Empty before first event. |
| `lastHookedInputState` | `map` | Latest input snapshot from payload/runtime fallback. | May be stale when unhooked. |
| `asyncJobsInFlight` | `int` | Number of queued/running async requests. | Lower-bounded to `0`. |
| `asyncMaxConcurrency` | `int` | Global async concurrency budget used to derive lane pools. | Clamped `[1, 64]`. |
| `asyncIoMaxConcurrency` | `int` | Derived concurrency of IO lane pool. | Lower-bounded to `1`. |
| `asyncUtilityMaxConcurrency` | `int` | Derived concurrency of utility lane pool. | Lower-bounded to `1`. |
| `asyncRenderMaxConcurrency` | `int` | Derived concurrency of render lane pool. | Lower-bounded to `1`. |
| `asyncQueueDepth` | `int` | Current queued request count across lanes. | Lower-bounded to `0`. |
| `asyncQueuePeakDepth` | `int` | Peak observed queue depth since process start. | Monotonic increasing. |
| `asyncQueueDepthLimit` | `int` | Per-lane queued depth limit for backpressure. | Clamped `[1, 16384]`. |
| `asyncBackpressureDropCount` | `uint64` | Number of requests rejected by backpressure. | Monotonic increasing. |
| `asyncMergedRequestCount` | `uint64` | Number of requests merged by coalescing. | Monotonic increasing. |
| `asyncCanceledRequestCount` | `uint64` | Number of requests finalized as canceled. | Monotonic increasing. |
| `performanceMetrics` | `map` | Aggregated async latency/queue metrics snapshot. | Schema: `lvrs.performance.v1`. |
| `performanceTraceCapacity` | `int` | Max retained performance trace events. | Clamped `[128, 16384]`. |
| `performanceTraceCount` | `int` | Current retained performance trace events. | Bounded by capacity. |
| `readTextCacheTtlMs` | `int` | TTL for in-memory text read cache. | Clamped `[100, 3600000]`. |
| `readTextCacheCapacityBytes` | `int64` | Max memory budget for text cache. | Clamped `[64, 536870912]`. |
| `readTextCacheBytes` | `int64` | Current text cache memory usage. | Lower-bounded to `0`. |
| `readTextCacheEntryCount` | `int` | Current text cache entry count. | Lower-bounded to `0`. |

## 3. Method Contract (Detailed)

### 3.1 File APIs

#### `saveTextFile(path, text): bool`

- Uses `QSaveFile` for atomic write semantics.
- UTF-8 encoding for payload bytes.
- Returns `false` and sets `lastError` on open/write/commit failure.

Failure cases:

- empty/blank `path`,
- directory missing and not creatable by caller workflow,
- permission denied,
- partial write/commit failure.

#### `readTextFile(path): string`

- Reads text file as UTF-8 bytes.
- Returns empty string on failure and sets `lastError`.

#### `ensureDir(path): bool`

- Creates directory recursively (`mkpath(".")`) if absent.
- Returns `true` when already exists.
- Returns `false` + `lastError` on failure.

#### `writableLocation(location): string`

- Direct bridge to `QStandardPaths::writableLocation`.
- Caller is responsible for validating non-empty path and creating directories.

### 3.2 Async APIs (multi-thread, UI non-blocking)

#### `saveTextFileAsync(path, text): qulonglong`

- Enqueues atomic UTF-8 text save on backend worker thread pool.
- Returns request id immediately.
- Emits:
  - `asyncRequestQueued(requestId, "saveTextFile", path)`
  - `asyncRequestFinished(...)` on completion.
- Completion `result` map includes:
  - `bytes`

#### `readTextFileAsync(path): qulonglong`

- Enqueues UTF-8 text read on backend worker thread pool.
- Returns request id immediately.
- Emits queued/finished signals.
- Completion `result` map includes:
  - `text`
  - `length`

#### `ensureDirAsync(path): qulonglong`

- Enqueues recursive directory ensure (`mkpath(".")`) on worker pool.
- Returns request id immediately.
- Completion `result` map includes:
  - `ensured` (`bool`)

#### `dispatchAsyncTask(taskName, payload = {}, delayMs = 0): qulonglong`

- General-purpose async task dispatcher for UI orchestration.
- Routes to lane pool by `payload.lane` or task-name prefix:
  - `io`
  - `utility`
  - `render`
- Runs with immediate execution by default (`delayMs` remains metadata compatibility field).
- Optional `payload.workMs` simulates bounded background work for orchestration/testing.
- Supports coalescing:
  - set `payload.coalesce=true` (default),
  - set `payload.coalesceKey` to merge duplicate queued work.
- Backpressure is applied when lane queue depth exceeds `asyncQueueDepthLimit`.
- `delayMs` is accepted as metadata input (`requestedDelayMs`) for compatibility.
- Returns request id immediately.
- Completion `result` map echoes payload and appends:
  - `taskName`
  - `delayMs` (always `0`)
  - `requestedDelayMs`
  - `lane`
  - `coalesced`
  - `mergedIntoRequestId` (when merged)

#### `setAsyncMaxConcurrency(value): void`

- Controls global concurrency budget.
- Runtime derives per-lane pool sizes (`io/utility/render`) from this value.

#### `setAsyncQueueDepthLimit(value): void`

- Controls lane-level queued depth backpressure threshold.
- Requests beyond limit are rejected with backpressure error.

#### `cancelAsyncRequest(requestId, reason = "Canceled by request"): bool`

- Marks request cancellation token.
- If request did not start, finishes immediately as canceled.
- If already running, final completion is normalized to canceled outcome.

#### `setPerformanceTraceCapacity(value): void`

- Controls retained P0 timeline trace size.
- Older entries are evicted FIFO when capacity shrinks.

#### `setReadTextCacheTtlMs(value): void`

- Controls TTL-based cache expiry for `readTextFile*`.
- Expired entries are pruned on access/update.

#### `setReadTextCacheCapacityBytes(value): void`

- Controls read-cache memory budget in bytes.
- Cache uses TTL + LRU-like oldest-access eviction when over budget.

### 3.3 Runtime hook lifecycle APIs

#### `hookUserEvents(): bool`

Lifecycle:

1. Resolve `RuntimeEvents` singleton.
2. Start runtime daemon.
3. Disconnect old hook links.
4. Connect to `RuntimeEvents::eventRecorded`.
5. Connect runtime destruction handler.
6. Ingest runtime `recentEvents()` into backend mirror.
7. Set `userEventHooked=true`.

Returns `false` when runtime singleton is unavailable.

#### `unhookUserEvents(): void`

- Disconnects event and destroyed connections.
- Clears runtime pointer.
- Sets `userEventHooked=false`.
- Does not clear existing mirrored event cache.

#### `clearHookedUserEvents(): void`

- Clears mirrored events, type counters, last-hooked snapshots.
- Emits `hookedEventsChanged`.

### 3.4 Query APIs

#### `hookedUserEvents(limit = -1): list`

- `limit <= 0` or `limit >= count` -> full list.
- Positive limit returns newest N entries.

#### `hookedUserEventSummary(): map`

Includes:

- hook status, event count, capacity,
- last event,
- current input snapshot,
- per-type counts,
- runtime event sequence (when runtime pointer is alive).

#### `currentUserInputState(): map`

- Returns live runtime input state if runtime pointer is available.
- Else returns last cached input state.

#### `performanceMetrics(): map`

Includes:

- schema/component/epoch,
- `asyncJobsInFlight`, `asyncMaxConcurrency`,
- `asyncIoMaxConcurrency`, `asyncUtilityMaxConcurrency`, `asyncRenderMaxConcurrency`,
- `asyncQueueDepth`, `asyncQueuePeakDepth`, `asyncQueueDepthLimit`,
- `asyncBackpressureDropCount`, `asyncMergedRequestCount`, `asyncCanceledRequestCount`,
- `performanceTraceCount`, `performanceTraceCapacity`,
- `readTextCacheTtlMs`, `readTextCacheCapacityBytes`, `readTextCacheBytes`, `readTextCacheEntryCount`,
- `asyncLaneMetrics` (`io/utility/render` queued/running/max/peak),
- per-operation latency summary (`avg`, `p50`, `p95`, `p99`, `max`, `failureRate`).

#### `recentPerformanceTrace(limit = -1): list`

- Returns newest performance timeline events.
- Event schema: `lvrs.performance.trace.v1`.
- Typical phases:
  - `queued`
  - `started`
  - `finished`
  - `canceled`

#### `clearPerformanceTrace(): void`

- Clears retained performance timeline events.

## 4. Mirrored Cache Semantics

Each mirrored event:

- copies runtime event map,
- adds `hookEpochMs` at mirror time,
- updates per-type counters,
- updates last-input snapshot from payload input when present,
- evicts oldest entries if capacity exceeded.

Eviction policy:

- strict FIFO by insertion order,
- decrements per-type counters for dropped entries.

Performance trace semantics:

- append-only sequence ids (`sequence`) in process scope,
- bounded FIFO retention by `performanceTraceCapacity`,
- phase ordering per request: `queued -> started -> finished`,
- cancellation is represented by `canceled` trace and `finished(canceled=true)`.

## 5. Error Model

Common `lastError` values by category:

| Category | Typical Trigger |
|---|---|
| argument validation | empty path input |
| filesystem | open/read/write/commit failure |
| runtime integration | runtime singleton unavailable |

`lastError` is overwritten on each new operation start; callers needing historical errors must persist externally.

## 6. Usage Patterns

### 6.1 Runtime-hook + export

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    if (!LV.Backend.hookUserEvents())
        console.warn("hook failed:", LV.Backend.lastError)
}

function exportEvents(path) {
    const payload = JSON.stringify(LV.Backend.hookedUserEvents(512), null, 2)
    if (!LV.Backend.saveTextFile(path, payload))
        console.warn("save failed:", LV.Backend.lastError)
}
```

### 6.2 Async non-blocking UI workflow

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.Backend.asyncMaxConcurrency = 4
}

Connections {
    target: LV.Backend
    function onAsyncRequestFinished(requestId, operation, subject, ok, result, error, elapsedMs) {
        if (!ok) {
            console.warn(operation, "failed:", error)
            return
        }
        if (operation === "readTextFile")
            console.log("loaded text:", result.text)
    }
}

function refreshModelFromFile(path) {
    LV.Backend.readTextFileAsync(path)
}
```

### 6.3 Measurement-window reset pattern

```qml
function beginMeasurementWindow() {
    LV.Backend.clearHookedUserEvents()
    if (!LV.Backend.userEventHooked)
        LV.Backend.hookUserEvents()
}
```

## 7. Failure/Troubleshooting Matrix

| Symptom | Likely Cause | Verification | Action |
|---|---|---|---|
| `hookUserEvents()` fails | runtime singleton missing | inspect `lastError` | ensure runtime module singleton exists before hook |
| mirrored events do not grow | not hooked or no runtime events | check `userEventHooked`, `hookedEventCount` | hook runtime and verify runtime started |
| per-type counts inconsistent expectation | capacity eviction occurred | compare count vs capacity | increase capacity for burst windows |
| stale input state | runtime detached/unhooked | check `userEventHooked` | re-hook or read direct runtime singleton |
| save fails intermittently | path/permission issue | inspect `lastError` content | validate directory + permission + disk state |
| UI stutter during file/task workflow | sync API was used in hot path | inspect calls for `saveTextFile/readTextFile/ensureDir` | migrate to async APIs and handle completion signals |

## 8. Codex-Oriented Playbook

### 8.1 Safe patch order for Codex

1. Preserve `hookUserEvents()` lifecycle order (resolve -> start -> reconnect -> ingest cache).
2. Preserve `hookedEventCapacity` clamping and FIFO eviction.
3. Keep `lastError` clearing behavior at operation entry points.
4. Keep schema compatibility in `hookedUserEventSummary()`.

### 8.2 Codex anti-patterns

- Do not auto-clear mirrored cache inside `unhookUserEvents()`; this changes API semantics.
- Do not block file APIs on runtime hook state.
- Do not remove `hookEpochMs`; downstream tooling may rely on it.

### 8.3 Codex regression checklist

After modifications:

1. hook/unhook toggles `userEventHooked` correctly,
2. capacity clamp + eviction behavior remains deterministic,
3. async queue emits queued/finished signals with stable schema,
4. summary map includes expected keys,
5. file save/read failure paths still set `lastError`,
6. performance trace sequence remains monotonic.

## 9. Validation Checklist

- Runtime hook succeeds with active runtime singleton.
- Mirrored count obeys capacity bounds.
- Type counters adjust on both append and eviction.
- Async jobs update `asyncJobsInFlight` correctly.
- File helper APIs behave atomically and report errors correctly.

## 10. Related APIs

- `RuntimeEvents`: source event daemon.
- `DebugLogger`: optional event/log stream observer.
- `ApplicationWindow`: common startup host that may trigger runtime hook flow.
