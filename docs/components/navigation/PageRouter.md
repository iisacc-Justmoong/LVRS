# PageRouter

Location: `qml/components/navigation/PageRouter.qml`

`PageRouter` is the LVRS stack navigation engine built on `StackView` + `RouteResolver`.

## Purpose

- Resolve route paths to page targets (`component`/`source`).
- Synchronize path stack metadata with visual stack operations.
- Integrate optional ViewModel/ViewState tracking hooks.

## Core API

Route config:

- `routes`
- `initialPath`
- `notFoundComponent`, `notFoundSource`
- `routeResolveCacheCapacity`

Stack and path state:

- `path` (SwiftUI-like stack entries)
- `currentPath`, `currentParams`
- `depth`, `canGoBack`, `currentPageItem`
- interactive transition state:
  - `interactiveTransitionActive`
  - `interactiveTransitionProgress`
  - `interactiveTransitionDirection`
  - `interactiveTransitionOperation`
  - `interactiveTransitionFromPath`, `interactiveTransitionToPath`
  - `interactiveTransitionFromParams`, `interactiveTransitionToParams`
  - `interactiveTransitionVelocityX`, `interactiveTransitionVelocityY`
  - `interactiveTransitionMeta`
  - `interactiveTransitionPreviewItem`
  - `interactiveTransitionCanCommit`

Presentation flags:

- `enforcePageViewport`
- `isolateInactivePages`
- `retainInactivePageCount`
- `interactiveTransitionsEnabled`
- `interactiveTransitionSettleDuration` (default: `0`)
- `interactiveTransitionCommitProgress`
- `interactiveTransitionVelocityThreshold`
- `interactiveTransitionOutgoingParallaxFactor`
- `interactiveTransitionIncomingPreviewFactor`

Global registration:

- `registerAsGlobalNavigator`

Signals:

- `navigated(path, params)`
- `navigationFailed(path)`
- `componentNavigated(component)`
- `interactiveTransitionStarted(state)`
- `interactiveTransitionUpdated(state)`
- `interactiveTransitionCommitted(state)`
- `interactiveTransitionCancelled(state)`
- `interactiveTransitionRejected(reason, state)`

Navigation methods:

- path-based: `go`, `push`, `replace`, `setRoot`, `back`, `pop`, `popToRoot`
- component-based: `goTo`, `replaceWith`, `setRootComponent`
- interactive:
  - `beginInteractiveTransition(spec)`
  - `beginInteractiveBack(meta?)`
  - `beginInteractivePush(path, params?, meta?)`
  - `beginInteractiveReplace(path, params?, meta?)`
  - `beginInteractiveSetRoot(path, params?, meta?)`
  - `beginInteractivePushComponent(component, params?, meta?)`
  - `beginInteractiveReplaceComponent(component, params?, meta?)`
  - `beginInteractiveSetRootComponent(component, params?, meta?)`
  - `updateInteractiveTransition(progress, details?)`
  - `shouldCommitInteractiveTransition(progress?, velocityX?, velocityY?)`
  - `finishInteractiveTransition(commit?)`
  - `cancelInteractiveTransition()`
  - `interactiveTransitionState()`

## Route Grammar

- static: `/reports`
- param: `/runs/[id]`
- rest: `/logs/[...path]`

## Behavior Contract

- Route resolution uses internal `RouteResolver` and cache capacity from `routeResolveCacheCapacity`.
- `navigate(...)` merges resolved params with caller params.
- Missing route falls back to not-found target when configured; otherwise emits `navigationFailed`.
- `applyPageViewportContract` and `applySingleChildViewportContract` enforce viewport-safe page sizing.
- When `isolateInactivePages == true`, non-retained stack items are hidden/disabled/opacity-zero.
- Router can sync bindings with `ViewModels` and snapshot with `ViewStateTracker` when available.
- Interactive transition orchestration is delegated to an internal driver object so `PageRouter` stays the source of committed stack truth.
- Interactive transitions do not mutate `path` or `currentPath` until `finishInteractiveTransition(true)` commits.
- Backward interactive transitions reuse the previous stack item as the preview surface.
- Forward interactive transitions instantiate a live preview item above the stack; preview pages therefore run ordinary QML lifecycle code before commit.
- Interactive transition commits suppress the built-in `StackView` push/pop animation and apply the stack mutation immediately, so the user-driven drag remains the only visible motion.
- `interactiveTransitionSettleDuration` defaults to `0`, so releasing an interactive gesture does not play an additional post-release settle animation unless a host explicitly opts into one.
- A competing non-interactive navigation call aborts the active interactive transition first.

## Usage

```qml
import LVRS 1.0 as LV

LV.PageRouter {
    id: router
    routes: [
        { path: "/", component: homePage },
        { path: "/runs/[id]", component: runPage }
    ]
    initialPath: "/"
}
```

Interactive back-swipe:

```qml
if (!router.interactiveTransitionActive)
    router.beginInteractiveBack({ source: "edge-pan" })

router.updateInteractiveTransition(progress, { velocityX: velocityX })
router.finishInteractiveTransition()
```
