# PageTransitionController

Location: `qml/components/navigation/PageTransitionController.qml`

`PageTransitionController` is a non-visual proxy that drives `PageRouter` interactive transitions while leaving committed route state inside the router.

## Purpose

- Provide a controller-shaped surface above `PageRouter`.
- Bind to a specific router or follow `Navigator.router`.
- Forward transition lifecycle signals into a dedicated object.

## Properties

- `router`
- `active`
- `progress`
- `direction`
- `operation`
- `fromPath`, `toPath`
- `fromParams`, `toParams`
- `canCommit`

## Signals

- `started(state)`
- `updated(state)`
- `committed(state)`
- `cancelled(state)`
- `rejected(reason, state)`

## Methods

- `canControl()`
- `begin(spec)`
- `beginBack(meta?)`
- `beginPush(path, params?, meta?)`
- `beginReplace(path, params?, meta?)`
- `beginSetRoot(path, params?, meta?)`
- `beginPushComponent(component, params?, meta?)`
- `beginReplaceComponent(component, params?, meta?)`
- `beginSetRootComponent(component, params?, meta?)`
- `update(progress, details?)`
- `finish(commit?)`
- `cancel()`
- `shouldCommit(progress?, velocityX?, velocityY?)`

## Behavior Contract

- The controller never mutates `PageRouter.path` directly; commit and cancel still happen inside the router.
- When `router` is not set, the default binding follows `Navigator.router`.
- `finish()` without an explicit boolean uses the router's commit heuristic.

## Usage

```qml
import LVRS 1.0 as LV

LV.PageTransitionController {
    id: transitions
    router: router

    function driveBackSwipe(progress, velocityX) {
        if (!active)
            beginBack({ source: "edge-pan" })
        update(progress, { velocityX: velocityX })
    }
}
```

## How It Works

- Readonly state is mirrored from the bound router's interactive transition properties.
- Lifecycle signals are forwarded through `Connections`.
- This keeps gesture policy and transition policy decoupled from committed routing state.
