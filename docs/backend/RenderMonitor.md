# RenderMonitor

Location: `backend/runtime/renderingmonitor.h` / `backend/runtime/renderingmonitor.cpp`

`RenderMonitor` provides frame timing metrics (`fps`, frame time, percentile frame stats, drop counters) for a `QQuickWindow`.

## Purpose

- Observe render cadence via `QQuickWindow::frameSwapped`.
- Expose runtime frame performance metrics to QML.
- Allow start/stop/reset control independent of window lifecycle.
- Provide P0 baseline metrics (`avg`, `p95`, `p99`, dropped frames) in a single snapshot schema.

## Properties

- `active: bool`
- `fps: double`
- `lastFrameMs: double`
- `avgFrameMs: double`
- `p95FrameMs: double`
- `p99FrameMs: double`
- `droppedFrameCount: uint64`
- `droppedFrameThresholdMs: double`
- `frameSampleCapacity: int`
- `recentSampleCount: int`
- `frameCount: uint64`

## Methods

- `attachWindow(window)`
- `start()`
- `stop()`
- `reset()`
- `performanceSnapshot()`

## Signals

- `activeChanged()`
- `statsChanged()`
- `droppedFrameThresholdMsChanged()`
- `frameSampleCapacityChanged()`

## How It Works

- `attachWindow(window)` disconnects existing target and binds to new `QQuickWindow`.
- On every `frameSwapped`, monitor computes elapsed milliseconds from previous frame.
- `fps` is derived as `1000 / lastFrameMs` when elapsed time is positive.
- Maintains rolling frame samples and computes `avg/p95/p99` from that window.
- Increments `droppedFrameCount` when frame time exceeds `droppedFrameThresholdMs`.
- Window destruction auto-detaches target and deactivates monitor.

### Snapshot Schema (`performanceSnapshot`)

Returned map keys:

- `schema` (`lvrs.performance.v1`)
- `component` (`RenderMonitor`)
- `epochMs`
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

## Usage Example

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.RenderMonitor.attachWindow(window)
    LV.RenderMonitor.start()
}

LV.Label {
    text: "FPS: " + LV.RenderMonitor.fps.toFixed(1)
    style: body
}
```

## Operational Usage Pattern

Recommended lifecycle:

1. attach window after root window creation,
2. set `frameSampleCapacity` and `droppedFrameThresholdMs` for target scenario,
3. start monitor when entering performance-sensitive screen,
4. reset metrics before measurement run,
5. stop monitor when leaving the screen to reduce unnecessary signal churn.

## Caveats

- `fps` reflects frame swap cadence, not end-to-end app latency.
- First frame after reset is used to initialize timing baseline.
- Percentile metrics are computed from rolling window, not full process lifetime.

## FAQ

Q. Why does FPS read 0 at startup?  
A. Metrics are initialized after first frame swap; values remain zero before frame cadence is established.
