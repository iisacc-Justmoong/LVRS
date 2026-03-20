# AppBootstrapWindow

Location: `qml/AppBootstrapWindow.qml`

`AppBootstrapWindow` is the reusable downstream app-root shell for the LVRS mobile/desktop bootstrap profile.

## Purpose

- Provide a standard visible root window for consumer projects.
- Keep mobile-safe viewport defaults without the oversized-height workaround.
- Auto-attach runtime monitoring at startup.
- Initialize the internal page stack from a seedable `initialRoutePath`.
- Register the internal router as the global navigator by default.

## Inherited Base

- Inherits `ApplicationWindow`.
- All `ApplicationWindow` properties, aliases, methods, and signals remain available.

## Added Defaults

- `visible: true`
- `navigationEnabled: false`
- `autoAttachRuntimeEvents: true`
- `internalRouterRegisterAsGlobalNavigator: true`
- `mobileOversizedHeightEnabled: false`
- `useInternalPageStack: true`

## Added Property

- `initialRoutePath: string` (default `"/"`)

`pageInitialPath` is bound to `initialRoutePath`, so downstream C++ can seed the first route through `QmlAppLaunchSpec::initialProperties`.

## Usage

```qml
import QtQuick
import LVRS 1.0 as LV

LV.AppBootstrapWindow {
    width: 430
    height: 932
    title: "MyApp"

    pageRoutes: [
        { path: "/", component: homePage }
    ]

    Component {
        id: homePage
        Item {}
    }
}
```

## Recommendation

- Use `AppBootstrapWindow` for new consumer app roots that want the standard LVRS bootstrap path.
- Use `ApplicationWindow` directly when a project needs to opt out of the bootstrap defaults or manage route/runtime policy manually.
