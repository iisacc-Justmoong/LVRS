# PageRouter

Location: `qml/components/navigation/PageRouter.qml`

Stack-based navigation with Svelte-like path syntax and SwiftUI-like stack control.

## Path Syntax
- `/runs/[id]` dynamic segment
- `/logs/[...path]` rest segment

## Stack Control
- `path`: array of paths for stack state
- `push(path)`, `pop()`, `popToRoot()`
- `setRoot(path)`
- `enforcePageViewport` (default `true`): pushed/replaced pages are forced to router viewport size.
- `isolateInactivePages` (default `true`): only the active page remains visible/enabled, preventing inactive-page bleed-through.

## Delegation
- `registerAsGlobalNavigator` defaults to `true`.
- When enabled, `Navigator` auto-targets this router.

## MVVM Route Meta
- `viewModelKey` or `modelKey`: ViewModel key to bind.
- `viewId`: optional logical view identifier.
- `writable` or `modelWritable`: whether the view owns write permission.

## Usage
```qml
LV.PageRouter {
    id: router
    routes: [
        { path: "/", component: homePage, viewModelKey: "Overview", writable: true },
        { path: "/runs/[id]", component: runPage, viewModelKey: "RunDetails" }
    ]
}

LV.Navigator.go("/runs/42")
```

## Practical Examples

### Example 1: Static + dynamic routes
```qml
import QtQuick
import LVRS 1.0 as LV

LV.PageRouter {
    id: router
    anchors.fill: parent
    initialPath: "/"
    routes: [
        { path: "/", component: homePage },
        { path: "/runs/[id]", component: runPage }
    ]
}

Component { id: homePage; Rectangle { color: LV.Theme.surfaceAlt } }
Component { id: runPage; Rectangle { color: LV.Theme.surfaceGhost } }
```

### Example 2: Not-found page and error signal
```qml
import QtQuick
import LVRS 1.0 as LV

LV.PageRouter {
    notFoundComponent: notFoundPage
    onNavigationFailed: (path) => console.warn("Missing route:", path)
}

Component {
    id: notFoundPage
    Rectangle {
        color: LV.Theme.surfaceGhost
        LV.Label { anchors.centerIn: parent; text: "404"; style: title }
    }
}
```

### Example 3: Route-level MVVM ownership metadata
```qml
import QtQuick
import LVRS 1.0 as LV

LV.PageRouter {
    routes: [
        { path: "/", component: dashboardPage, viewModelKey: "DashboardVM", writable: true },
        { path: "/reports", component: reportsPage, viewModelKey: "ReportsVM", writable: false }
    ]
}

Component { id: dashboardPage; Rectangle {} }
Component { id: reportsPage; Rectangle {} }
```
