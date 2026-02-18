# Performance Observability (P0)

Location: `backend/io/backend.*` / `backend/runtime/renderingmonitor.*`

This document defines the P0 performance telemetry contract used by LVRS.

## Purpose

- Establish one shared metrics schema for CPU/GPU-adjacent runtime telemetry.
- Provide per-request async timeline traces for queue/start/finish analysis.
- Keep sampling overhead bounded and predictable for debug builds.

## Schemas

### 1) Snapshot Schema: `lvrs.performance.v1`

Used by:

- `Backend.performanceMetrics()`
- `RenderMonitor.performanceSnapshot()`

Common required fields:

- `schema`
- `component`
- `epochMs`

Backend-specific fields:

- `asyncJobsInFlight`
- `asyncMaxConcurrency`
- `asyncIoMaxConcurrency`
- `asyncUtilityMaxConcurrency`
- `asyncRenderMaxConcurrency`
- `asyncQueueDepth`
- `asyncQueuePeakDepth`
- `asyncQueueDepthLimit`
- `asyncBackpressureDropCount`
- `asyncMergedRequestCount`
- `asyncCanceledRequestCount`
- `performanceTraceCount`
- `performanceTraceCapacity`
- `readTextCacheTtlMs`
- `readTextCacheCapacityBytes`
- `readTextCacheBytes`
- `readTextCacheEntryCount`
- `asyncLaneMetrics`
- `asyncLatencyByOperation` (`avg/p50/p95/p99/max/failureRate`)

RenderMonitor-specific fields:

- `active`
- `fps`
- `lastFrameMs`
- `avgFrameMs`
- `p95FrameMs`
- `p99FrameMs`
- `frameCount`
- `droppedFrameCount`
- `droppedFrameThresholdMs`
- `recentSampleCount`
- `frameSampleCapacity`

### 2) Timeline Trace Schema: `lvrs.performance.trace.v1`

Used by:

- `Backend.recentPerformanceTrace()`

Required fields:

- `schema`
- `sequence` (monotonic in-process)
- `epochMs`
- `phase` (`queued`, `started`, `finished`, `canceled`)
- `requestId`
- `operation`
- `subject`
- `detail` (phase-specific map)

## Timeline Semantics

- `queued`: request accepted and `requestId` assigned.
- `started`: worker thread started actual task body.
- `finished`: task completed and completion signal was emitted.
- `canceled`: cancellation token was requested.

Expected order per request:

1. `queued`
2. `started` (may be absent for immediate-fast path)
3. `finished`

## Overhead Policy

- Trace retention is bounded by `performanceTraceCapacity`.
- Latency samples are bounded per operation (`m_asyncLatencySampleCapacity`).
- Render percentiles are bounded by rolling sample capacity (`frameSampleCapacity`).

## Operational Usage

1. Capture `Backend.performanceMetrics()` and `RenderMonitor.performanceSnapshot()` together for a measurement window.
2. Export `Backend.recentPerformanceTrace()` for queue/wait bottleneck analysis.
3. Compare p95/p99 across revisions, not only averages.
