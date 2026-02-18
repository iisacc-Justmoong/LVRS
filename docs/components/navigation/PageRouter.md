# PageRouter

Location: `qml/components/navigation/PageRouter.qml`

`PageRouter` is the stack navigation engine for LVRS, combining path grammar, StackView operations, optional view-model binding metadata, and view-state tracking.

## Purpose

- Provide push/replace/root navigation with path metadata.
- Resolve static/param/rest routes.
- Keep path stack (`path`) synchronized with visual stack.
- Integrate with `Navigator`, `ViewModels`, and `ViewStateTracker`.

## Core API

Route config:

- `routes`
- `initialPath`
- `notFoundComponent`
- `notFoundSource`

Stack state:

- `path`
- `currentPath`
- `currentParams`
- `canGoBack`
- `depth`
- `currentPageItem`

Behavior flags:

- `registerAsGlobalNavigator`
- `enforcePageViewport`
- `isolateInactivePages`
- `retainInactivePageCount`
- `routeResolveCacheCapacity`

Signals:

- `navigated(path, params)`
- `navigationFailed(path)`
- `componentNavigated(component)`

Navigation methods:

- `go(path, params)` / `push(path, params)`
- `replace(path, params)`
- `setRoot(path, params)`
- `back()` / `pop()`
- `popToRoot()`
- `goTo(component, params)`
- `replaceWith(component, params)`
- `setRootComponent(component, params)`

## Path Grammar

- static segment: `/reports`
- dynamic segment: `/runs/[id]`
- rest segment: `/logs/[...path]`

Path normalization enforces leading slash and removes trailing slash except root.

## Route Metadata for MVVM

Per route, optional metadata can include:

- `viewModelKey` or `modelKey`
- `viewId`
- `writable` or `modelWritable`

When provided and `ViewModels` exists, router auto-calls `ViewModels.bindView(...)` during navigation.

## Usage

```qml
import QtQuick
import LVRS 1.0 as LV

LV.PageRouter {
    id: router
    routes: [
        { path: "/", component: homePage, viewModelKey: "HomeVM", writable: true },
        { path: "/runs/[id]", component: runPage }
    ]
}
```

## How It Works

Stack operation flow:

1. Resolve route by normalized path.
2. Merge route params with caller params.
3. Apply StackView operation (`push`, `replace`, `set`).
4. Update `path` array with normalized path entry.
5. Update `currentPath/currentParams` and emit navigation signal.

Presentation flow:

- When `enforcePageViewport == true`, new page items are constrained to router viewport.
- When `isolateInactivePages == true`, only top page stays visible/enabled.
- `retainInactivePageCount > 0`이면 top 아래 N개 페이지를 함께 유지하고 나머지는 culling한다.

Route resolve flow:

- 경로 정규화/매칭은 `RouteMatcher`(C++ 싱글턴) 우선 경로를 사용한다.
- `routeResolveCacheCapacity` 기반의 route-resolve LRU 캐시로 반복 탐색 비용을 줄인다.

Tracker flow:

- Router builds tracking entries from `path`.
- Synchronizes `ViewModels` bindings and `ViewStateTracker` state (when globally active router).

## Failure Behavior

- If route missing and no not-found fallback: emits `navigationFailed(path)`.
- If route exists but component/source missing: emits `navigationFailed(path)`.

## Advanced Example: Dynamic + Rest Segment Route Set

```qml
import QtQuick
import LVRS 1.0 as LV

LV.PageRouter {
    routes: [
        { path: "/runs/[id]", component: runPage, viewModelKey: "RunVM" },
        { path: "/logs/[...path]", component: logPage, viewModelKey: "LogVM" }
    ]
}
```

`/logs/a/b/c` resolves `params.path = "a/b/c"` by rest-segment rule.

## Troubleshooting Checklist

- route not matched: verify normalized path and segment token syntax (`[id]`, `[...path]`),
- no page item shown: verify route has `component` or `source`,
- model binding absent: verify `viewModelKey/modelKey` exists in `ViewModels` registry.
- route 변경 후 이전 매칭이 남는 경우: `routes` 변경 시 cache는 자동 비우기 된다.

## Route Authoring Guidelines

- Keep route params names stable; treat rename as breaking API for dependent pages.
- Avoid overloading one route path with unrelated page semantics.
- Prefer explicit not-found component for user-visible fallback rather than silent failure logs.

## Observability Hooks

Instrument these signals in app diagnostics panel:

- `navigated(path, params)`
- `navigationFailed(path)`
- `componentNavigated(component)`
