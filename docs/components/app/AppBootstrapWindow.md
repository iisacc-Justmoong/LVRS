# AppBootstrapWindow

Location: `qml/AppBootstrapWindow.qml`

`AppBootstrapWindow` is a compatibility wrapper around `ApplicationWindow`.

## Purpose

- Preserve the older bootstrap-root import path for downstream apps.
- Provide a visible-root convenience default on top of the inherited `ApplicationWindow` bootstrap contract.

## Inherited Base

- Inherits `ApplicationWindow`.
- All `ApplicationWindow` properties, aliases, methods, and signals remain available, including `initialRoutePath`, page-stack hosting, and runtime-profile-driven defaults.

## Added Default

- `visible: true`

## Usage

```qml
import QtQuick
import LVRS 1.0 as LV

LV.AppBootstrapWindow {
    width: 900
    height: 620
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

- Use `ApplicationWindow` directly for new consumer app roots.
- Keep `AppBootstrapWindow` only when an existing codebase benefits from the legacy type name or wants `visible: true` preconfigured in the QML root.
- The inherited render path is the same runtime-direct `RenderQuality` policy as `ApplicationWindow`; automatic device-tier presets stay opt-in.
- On desktop targets the inherited default profile still auto-attaches `RuntimeEvents`; on iOS/Android it stays off unless the app opts in.
