# AppScaffold

Location: `qml/components/layout/AppScaffold.qml`

`AppScaffold` is an adaptive navigation + content container.
It does not force a top app header/title region.
Main content is full-bleed by default (`adaptiveOuterMargin: 0`, `adaptiveContentInset: 0`).

## Layout Model

- Navigation modes:
  - `rail` (desktop-wide)
  - `drawer`
  - `bottom`
  - `none`
- Layout profiles:
  - `mobile-compact`, `mobile-wide`, `desktop-compact`, `desktop-wide`
- The scaffold computes a requested state and applies guarded transitions.

## Main Properties

Navigation model:
- `navModel`
- `navIndex`
- `navigationEnabled`
- `navTitle`, `navTitleVisible`
- `navDelegate`, `navHeader`, `navFooter`

Navigation dimensions:
- `navWidth`
- `navDrawerWidth`
- `navRailMaxWidthRatio`
- `drawerMarginSafety`

Adaptive controls:
- `layoutMode` (`auto`, `mobile`, `desktop`)
- `layoutPlatform`
- `forceDesktopOnLargeMobile`
- `mobileDesktopMinWidth`
- `preferBottomNavigation`
- `bottomNavigationMaxItems`
- `compactSpacingEnabled`
- `compactSpacingBreakpoint`

Routing:
- `pageRouter` (external router)
- `routes`, `initialPath`
- `useInternalPageStack`
- `internalRouterRegisterAsGlobalNavigator`

State/read-only:
- `layoutProfile`, `navigationMode`
- `mobileLayout`, `desktopLayout`
- `navigationRailEnabled`, `drawerNavigationEnabled`, `bottomNavigationEnabled`
- `internalPageStackEnabled`
- `activePageRouter`

## Signals

- `navActivated(index, item)`
- `layoutStateChanged(profile, navigationMode)`
- `stackNavigated(path, params)`
- `stackNavigationFailed(path)`
- `transitionRejected(kind, fromState, toState, fallbackState)`

## Usage
```qml
import QtQuick
import LVRS 1.0 as LV

LV.AppScaffold {
    anchors.fill: parent
    navModel: [
        { label: "Overview", path: "/" },
        { label: "Reports", path: "/reports" }
    ]

    Rectangle {
        anchors.fill: parent
        color: LV.Theme.window
    }
}
```

## Practical Example: External Router
```qml
import QtQuick
import LVRS 1.0 as LV

Item {
    LV.PageRouter {
        id: router
        anchors.fill: parent
        initialPath: "/"
        routes: [
            { path: "/", component: overviewPage },
            { path: "/reports", component: reportsPage }
        ]
    }

    LV.AppScaffold {
        anchors.fill: parent
        pageRouter: router
        navModel: [
            { label: "Overview", path: "/" },
            { label: "Reports", path: "/reports" }
        ]
    }

    Component { id: overviewPage; Rectangle { color: LV.Theme.surfaceAlt } }
    Component { id: reportsPage; Rectangle { color: LV.Theme.surfaceGhost } }
}
```
