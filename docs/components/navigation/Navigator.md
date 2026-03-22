# Navigator

Location: `qml/components/navigation/Navigator.qml`

`Navigator` is the global singleton delegate for active `PageRouter` navigation.

## Purpose

- Allow one-line app-wide navigation calls.
- Track router registration stack and select active router.
- Proxy path/component navigation and back stack operations.

## Properties

- `router`
- `routerStack`
- `hasRouter`
- `currentPath`
- `depth`
- `interactiveTransitionActive`
- `interactiveTransitionProgress`
- `interactiveTransitionDirection`
- `interactiveTransitionOperation`

## Router Lifecycle Methods

- `registerRouter(targetRouter)`
- `unregisterRouter(targetRouter?)`

Registration order considers sibling order when routers share parent, then falls back to append.

## Navigation Methods

Path-based:

- `go(path, params)`
- `replace(path, params)`
- `setRoot(path, params)`
- `back()`
- `popToRoot()`

Component-based:

- `goTo(component, params)`
- `replaceWith(component, params)`
- `setRootComponent(component, params)`

Interactive:

- `beginInteractiveTransition(spec)`
- `beginInteractiveBack(meta?)`
- `beginInteractivePush(path, params?, meta?)`
- `beginInteractiveReplace(path, params?, meta?)`
- `beginInteractiveSetRoot(path, params?, meta?)`
- `updateInteractiveTransition(progress, details?)`
- `finishInteractiveTransition(commit?)`
- `cancelInteractiveTransition()`
- `shouldCommitInteractiveTransition(progress?, velocityX?, velocityY?)`

## Usage

```qml
import LVRS 1.0 as LV

LV.LabelButton {
    text: "Open Reports"
    onClicked: LV.Navigator.go("/reports")
}
```

## How It Works

- Active router is the tail of `routerStack`.
- Every navigation method returns `false` when no capable router exists.
- On router activation change, tracker sync hook (`syncViewStateTracker`) is invoked when supported.
- Interactive transition helpers stay as thin proxies; preview creation and stack commit remain router responsibilities.

## Nested Router Strategy

In applications with nested routers, register/unregister order determines active router resolution.
Use explicit registration boundaries when mounting/unmounting nested navigation regions.

## Debug Tip

Check `Navigator.currentPath` and `Navigator.depth` in debug panels to verify global target router state before diagnosing route failures.

## Recipe: Safe Back Action

```qml
import LVRS 1.0 as LV

function safeBack() {
    if (LV.Navigator.depth > 1)
        LV.Navigator.back()
}
```

Interactive edge-swipe via the active router:

```qml
import LVRS 1.0 as LV

function driveEdgePan(progress, velocityX) {
    if (!LV.Navigator.interactiveTransitionActive)
        LV.Navigator.beginInteractiveBack({ source: "edge-pan" })
    LV.Navigator.updateInteractiveTransition(progress, { velocityX: velocityX })
}
```

## Failure Pattern

Calling `back()` without depth guard in single-entry stacks produces no-op and can hide navigation bugs in upstream flow design.
