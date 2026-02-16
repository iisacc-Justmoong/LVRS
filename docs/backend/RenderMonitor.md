# RenderMonitor

Location: `backend/runtime/renderingmonitor.h` / `backend/runtime/renderingmonitor.cpp`

`RenderMonitor` provides frame timing metrics (`fps`, frame time, frame count) for a `QQuickWindow`.

## Purpose

- Observe render cadence via `QQuickWindow::frameSwapped`.
- Expose runtime frame performance metrics to QML.
- Allow start/stop/reset control independent of window lifecycle.

## Properties

- `active: bool`
- `fps: double`
- `lastFrameMs: double`
- `frameCount: uint64`

## Methods

- `attachWindow(window)`
- `start()`
- `stop()`
- `reset()`

## Signals

- `activeChanged()`
- `statsChanged()`

## How It Works

- `attachWindow(window)` disconnects existing target and binds to new `QQuickWindow`.
- On every `frameSwapped`, monitor computes elapsed milliseconds from previous frame.
- `fps` is derived as `1000 / lastFrameMs` when elapsed time is positive.
- Window destruction auto-detaches target and deactivates monitor.

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
2. start monitor when entering performance-sensitive screen,
3. reset metrics before measurement run,
4. stop monitor when leaving the screen to reduce unnecessary signal churn.

## Caveats

- `fps` reflects frame swap cadence, not end-to-end app latency.
- First frame after reset is used to initialize timing baseline.

## FAQ

Q. Why does FPS read 0 at startup?  
A. Metrics are initialized after first frame swap; values remain zero before frame cadence is established.
