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

Presentation flags:

- `enforcePageViewport`
- `isolateInactivePages`
- `retainInactivePageCount`

Global registration:

- `registerAsGlobalNavigator`

Signals:

- `navigated(path, params)`
- `navigationFailed(path)`
- `componentNavigated(component)`

Navigation methods:

- path-based: `go`, `push`, `replace`, `setRoot`, `back`, `pop`, `popToRoot`
- component-based: `goTo`, `replaceWith`, `setRootComponent`

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
