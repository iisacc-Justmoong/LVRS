# Link

Location: `qml/components/navigation/Link.qml`

`Link` is a click-to-navigation control that wraps route/component navigation into a button-like API.

## Purpose

- Provide declarative navigation trigger without direct router method calls in each click handler.
- Support route path navigation and component-target navigation in one component.

## API

Routing:

- `router` (optional explicit router)
- `href`
- `to` (alias of `href`)
- `params`
- `replace`
- `targetComponent`

Visual:

- `linkColor`
- `hoverColor`
- `disabledColor`
- `underline`

Content:

- default `content` slot
- fallback text rendering when no slot child exists

## Router Resolution

`Link` chooses router in this order:

1. explicit `router` property
2. `Navigator.router`
3. no-op if unresolved

## Navigation Behavior

- if `targetComponent` is set:
  - `replace == true` -> `replaceWith(component, params)`
  - else -> `goTo(component, params)`
- else route path:
  - `replace == true` -> `replace(href, params)`
  - else -> `go(href, params)`

## Usage

```qml
import LVRS 1.0 as LV

LV.Link {
    href: "/reports"
    text: "Open Reports"
    underline: true
}
```

## Advanced Example: Component Navigation Replacement

```qml
import QtQuick
import LVRS 1.0 as LV

Item {
    Component { id: inspectorPage; Rectangle {} }

    LV.Link {
        targetComponent: inspectorPage
        replace: true
        params: ({ source: "sidebar" })
        text: "Open Inspector"
    }
}
```

## Common Mistake

When no explicit `router` is provided, navigation silently no-ops if `Navigator.router` is not registered.
Ensure router registration exists in nested/embedded navigation setups.

## Recipe: Route Replace for Wizard Step

```qml
import LVRS 1.0 as LV

LV.Link {
    href: "/wizard/step-2"
    replace: true
    text: "Next"
}
```

Use `replace` to prevent back-stack noise in linear wizard flows.
