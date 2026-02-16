# EventListener

Location: `qml/components/control/util/EventListener.qml`

`EventListener` is an embeddable behavior bridge that maps trigger names to pointer/wheel/key/global runtime callbacks.

## Purpose

- Replace ad-hoc event glue with one payload contract.
- Provide optional backend-first input snapshot resolution.
- Support dedup windows for global press/context behavior.

## Trigger Set

- pointer-local: `clicked`, `pressed`, `released`, `entered`, `exited`, `hoverChanged`
- wheel: `wheel`
- keyboard: `keyPressed`, `keyReleased`
- global runtime: `globalPressed`, `globalContextRequested`

## Core API

- `trigger`
- `action` (callback)
- `enabled`
- `acceptedButtons`
- `includeUiHit` (default `true`)
- `preferBackendState` (default `false`)
- `includeBackendSummary` (default `false`)
- `macControlClickAsRight`

Dedup controls:

- `globalPressDedupMs`, `globalPressDedupTolerancePx`
- `contextDedupMs`, `contextDedupTolerancePx`

## Payload Behavior

Global payloads may include:

- position fields (`x`, `y`, `globalX`, `globalY`)
- button/modifier masks
- `input` snapshot
- `ui` hit-test map when enabled
- `backend` summary when requested

## Usage

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "globalContextRequested"
    preferBackendState: true
    action: function(eventData) {
        console.log(eventData.globalX, eventData.globalY)
    }
}
```

## How It Works

- Trigger string selects source path (`MouseArea`, `WheelHandler`, `Keys`, or runtime `Connections`).
- Backend-first mode calls `Backend.hookUserEvents()` when needed and resolves `Backend.currentUserInputState()` first.
- Context dedup suppresses duplicate context callbacks within configured time-distance window.

## Advanced Example: Backend Summary Included

```qml
import LVRS 1.0 as LV

LV.EventListener {
    trigger: "globalPressed"
    preferBackendState: true
    includeBackendSummary: true
    action: function(eventData) {
        console.log(eventData.backend.eventCount)
    }
}
```

## Troubleshooting

If trigger callback is never invoked:

1. verify `enabled` state,
2. verify trigger token spelling,
3. verify runtime event source availability for global triggers.

## Review Checklist

- trigger token matches supported set,
- callback is pure and non-blocking,
- dedup thresholds are justified for context/press flows,
- backend-first mode is enabled only when stable snapshot semantics are required.
