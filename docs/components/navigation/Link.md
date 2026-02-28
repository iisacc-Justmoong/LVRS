# Link

Location: `qml/components/navigation/Link.qml`

`Link` is a navigation trigger component built on `AbstractButton`.

## Purpose

- Provide declarative route/component navigation without per-view router boilerplate.
- Support both path navigation and component navigation with optional replace semantics.

## Core API

Routing:

- `router` (optional explicit router)
- `href`
- `to` (alias of `href`)
- `params`
- `replace`
- `targetComponent`

Visual:

- `linkColor`, `hoverColor`, `pressedColor`, `disabledColor`
- `underline`

Content:

- default `content` slot
- text fallback label when slot is empty

## Behavior Contract

Router resolution order:

1. `router`
2. `Navigator.router`
3. no-op when unresolved

Navigation behavior:

- if `targetComponent` is set:
  - `replace == true` -> `replaceWith(targetComponent, params)`
  - else -> `goTo(targetComponent, params)`
- else if `href` exists:
  - `replace == true` -> `replace(href, params)`
  - else -> `go(href, params)`

## Usage

```qml
import LVRS 1.0 as LV

LV.Link {
    href: "/reports"
    text: "Reports"
}
```
